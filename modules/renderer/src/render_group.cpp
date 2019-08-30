// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/core_util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    using namespace glext;

    extern Shader g_group_transform_shader;

    static std::vector<const Shader*> _generate_initial_group_shaders(void) {
        std::vector<const Shader*> shaders;
        shaders.insert(shaders.cbegin(), &g_group_transform_shader);
        return shaders;
    }

    ShaderProgram RenderGroup::generate_initial_program(void) {
        std::vector<const Shader*> final_shaders;
        final_shaders.reserve(parent.shaders.size() + shaders.size());
        if (parent.shaders.size() > 0) {
            std::copy(parent.shaders.begin(), parent.shaders.end(), std::back_inserter(final_shaders));
        }
        if (shaders.size() > 0) {
            std::copy(shaders.begin(), shaders.end(), std::back_inserter(final_shaders));
        }
        return ShaderProgram(std::move(final_shaders));
    }

    RenderGroup::RenderGroup(RenderLayer &parent):
            parent(parent),
            renderable_factory(RenderableFactory(*this)),
            shaders(_generate_initial_group_shaders()),
            shader_program(generate_initial_program()),
            dirty_children(false),
            dirty_shaders(false),
            buffers_initialized(false),
            shaders_initialized(false) {
    }

    void RenderGroup::destroy(void) {
        if (&parent.root_group == this) {
            _ARGUS_FATAL("Cannot destroy root RenderGroup");
        }

        parent.remove_group(*this);

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    }

    Transform &RenderGroup::get_transform(void) {
        return transform;
    }

    void RenderGroup::update_buffer(void) {
        // if the children list is dirty, we'll just reinitialize the buffer entirely
        if (!buffers_initialized || dirty_children) {
            if (buffers_initialized) {
                glDeleteVertexArrays(1, &vao);
            }

            // init vertex array
            glGenVertexArrays(1, &vao);
        }

        glBindVertexArray(vao);

        if (!buffers_initialized || dirty_children) {
            if (buffers_initialized) {
                glDeleteBuffers(1, &vbo);
            }

            // init vertex buffer
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            GLenum err = glGetError();

            // compute how many vertices will be in this buffer
            vertex_count = 0;
            for (Renderable *const child : this->children) {
                vertex_count += child->get_vertex_count();
            }

            // allocate a new buffer
            glBufferData(GL_ARRAY_BUFFER, vertex_count * _VERTEX_LEN * _VERTEX_WORD_LEN, nullptr, GL_DYNAMIC_DRAW);

            // set up attribute metadata
            glEnableVertexAttribArray(_ATTRIB_LOC_POSITION);
            glEnableVertexAttribArray(_ATTRIB_LOC_COLOR);
            glEnableVertexAttribArray(_ATTRIB_LOC_TEXCOORD);

            GLintptr position_offset = 0;
            GLintptr color_offset = (_VERTEX_POSITION_LEN) * _VERTEX_WORD_LEN;
            GLintptr texcoord_offset = (_VERTEX_POSITION_LEN + _VERTEX_COLOR_LEN) * _VERTEX_WORD_LEN;
            GLint vertex_stride = _VERTEX_LEN * _VERTEX_WORD_LEN;

            glVertexAttribPointer(_ATTRIB_LOC_POSITION, _VERTEX_POSITION_LEN, GL_FLOAT, GL_FALSE,
                    vertex_stride, reinterpret_cast<GLvoid*>(position_offset));
            glVertexAttribPointer(_ATTRIB_LOC_COLOR, _VERTEX_COLOR_LEN, GL_FLOAT, GL_FALSE,
                    vertex_stride, reinterpret_cast<GLvoid*>(color_offset));
            glVertexAttribPointer(_ATTRIB_LOC_TEXCOORD, _VERTEX_TEXCOORD_LEN, GL_FLOAT, GL_FALSE,
                     vertex_stride, reinterpret_cast<GLvoid*>(texcoord_offset));
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
        }

        // push each child's data to the buffer, if applicable
        // we only push a child's data if the child list has changed, or the specific child's transform has changed
        size_t offset = 0;
        for (Renderable *const child : this->children) {
            if (child->transform.is_dirty() || dirty_children) {
                child->allocate_buffer(child->get_vertex_count() * _VERTEX_LEN);
                child->populate_buffer();
                child->upload_buffer(offset);
                child->transform.clean();
            }
            offset += child->get_vertex_count();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        if (dirty_children) {
            dirty_children = false;
        }

        if (!buffers_initialized) {
            buffers_initialized = true;
        }
    }

    void RenderGroup::refresh_shaders(void) {
        if (!shaders_initialized || parent.dirty_shaders || dirty_shaders) {
            if (shaders_initialized) {
                glDeleteProgram(shader_program.program_handle);
            }

            std::vector<const Shader*> new_shaders;
            if (!shaders_initialized || parent.dirty_shaders) {
                new_shaders.insert(new_shaders.end(), parent.shaders.begin(), parent.shaders.end());
            }

            if (!shaders_initialized || dirty_shaders) {
                new_shaders.insert(new_shaders.end(), shaders.begin(), shaders.end());
            }

            shader_program.update_shaders(new_shaders);
            shader_program.link();
        } else if (shader_program.needs_rebuild) {
            shader_program.link();
        }

        if (!shaders_initialized || transform.is_dirty() || parent.transform.is_dirty()) {
            glUseProgram(shader_program.program_handle);
        }

        if (!shaders_initialized || transform.is_dirty()) {
            float transform_matrix[16];
            transform.to_matrix(transform_matrix);
            glUniformMatrix4fv(shader_program.get_uniform_location(_UNIFORM_GROUP_TRANSFORM), 1, GL_FALSE, transform_matrix);
            transform.clean();
        }

        if (!shaders_initialized || parent.transform.is_dirty()) {
            float transform_matrix[16];
            parent.transform.to_matrix(transform_matrix);
            glUniformMatrix4fv(shader_program.get_uniform_location(_UNIFORM_LAYER_TRANSFORM), 1, GL_FALSE, transform_matrix);
        }

        if (!shaders_initialized || transform.is_dirty() || parent.transform.is_dirty()) {
            glUseProgram(0);
        }

        if (dirty_shaders) {
            dirty_shaders = false;
        }

        if (!shaders_initialized) {
            shaders_initialized = true;
        }
    }

    void RenderGroup::draw(void) {
        refresh_shaders();

        glUseProgram(shader_program.program_handle);
        
        update_buffer();

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertex_count));
        glBindVertexArray(0);

        glUseProgram(0);
    }

    void RenderGroup::add_renderable(Renderable &renderable) {
        children.insert(children.cbegin(), &renderable);
        dirty_children = true;
    }

    void RenderGroup::remove_renderable(Renderable &renderable) {
        _ARGUS_ASSERT(&renderable.parent == this, "remove_renderable was passed Renderable with wrong parent");

        remove_from_vector(children, &renderable);
        delete &renderable;
        dirty_children = true;
    }

    RenderableFactory &RenderGroup::get_renderable_factory(void) {
        return renderable_factory;
    }

}

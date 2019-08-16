// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/defines.hpp"
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
        if (parent.shaders.size() > 0) {
            final_shaders.insert(final_shaders.cbegin(), parent.shaders.cbegin(), parent.shaders.cend());
        }
        if (shaders.size() > 0) {
            final_shaders.insert(final_shaders.cbegin(), shaders.cbegin(), shaders.cend());
        }
        return ShaderProgram(final_shaders);
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
            glBufferData(GL_ARRAY_BUFFER, vertex_count * __VERTEX_LEN * __VERTEX_WORD_LEN, nullptr, GL_DYNAMIC_DRAW);

            // set up attribute metadata
            glEnableVertexAttribArray(__ATTRIB_LOC_POSITION);
            glEnableVertexAttribArray(__ATTRIB_LOC_COLOR);
            glEnableVertexAttribArray(__ATTRIB_LOC_TEXCOORD);

            glVertexAttribPointer(__ATTRIB_LOC_POSITION, __VERTEX_POSITION_LEN, GL_FLOAT, false, __VERTEX_LEN * __VERTEX_WORD_LEN);
            glVertexAttribPointer(__ATTRIB_LOC_COLOR, __VERTEX_COLOR_LEN, GL_FLOAT, false, __VERTEX_LEN * __VERTEX_WORD_LEN);
            glVertexAttribPointer(__ATTRIB_LOC_TEXCOORD, __VERTEX_TEXCOORD_LEN, GL_FLOAT, false, __VERTEX_LEN * __VERTEX_WORD_LEN);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        // push each child's data to the buffer, if applicable
        // we only push a child's data if the child list has changed, or the specific child's transform has changed
        size_t offset = 0;
        for (Renderable *const child : this->children) {
            if (child->transform.is_dirty() || dirty_children) {
                child->render(vbo, offset);
                child->transform.clean();
            }
            offset += child->get_vertex_count();
        }

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
                glDeleteProgram(shader_program.gl_program);
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
        }

        if (!shaders_initialized || transform.is_dirty() || parent.transform.is_dirty()) {
            glUseProgram(shader_program.gl_program);
        }

        if (!shaders_initialized || transform.is_dirty()) {
            glUniformMatrix4fv(shader_program.get_uniform_location(__UNIFORM_GROUP_TRANSFORM), 1, GL_TRUE,
                    transform.to_matrix().array);
            transform.clean();
        }

        if (!shaders_initialized || parent.transform.is_dirty()) {
            glUniformMatrix4fv(shader_program.get_uniform_location(__UNIFORM_LAYER_TRANSFORM), 1, GL_TRUE,
                    parent.transform.to_matrix().array);
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
        printf("draw\n");
        if (shader_program.needs_rebuild) {
            shader_program.link();
        }

        glUseProgram(shader_program.gl_program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
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

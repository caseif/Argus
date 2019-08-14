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

    static std::vector<Shader*> _generate_initial_shaders(void) {
        std::vector<Shader*> shaders;
        shaders.insert(shaders.cbegin(), &g_group_transform_shader);
        return shaders;
    }

    static ShaderProgram _generate_iniital_program(std::vector<Shader*> &parent_shaders, std::vector<Shader*> &self_shaders) {
        std::vector<Shader*> final_shaders;
        final_shaders.insert(final_shaders.cbegin(), parent_shaders.cbegin(), parent_shaders.cend());
        final_shaders.insert(final_shaders.cbegin(), self_shaders.cbegin(), self_shaders.cend());
        return ShaderProgram(final_shaders);
    }

    RenderGroup::RenderGroup(RenderLayer &parent):
            parent(parent),
            renderable_factory(*new RenderableFactory(*this)),
            shaders(_generate_initial_shaders()),
            shader_program(_generate_iniital_program(parent.shaders, shaders)) {
    }

    void RenderGroup::destroy(void) {
        remove_from_vector(parent.children, this);

        delete this;
    }

    Transform const &RenderGroup::get_transform(void) const {
        return transform;
    }

    void RenderGroup::update_buffer(void) {
        // if the children list is dirty, we'll just reinitialize the buffer entirely
        if (dirty_children) {
            // init vertex array
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            // init vertex buffer
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

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

        dirty_children = false;
    }

    void RenderGroup::refresh_shaders(void) {
        if (parent.dirty_shaders || dirty_shaders) {
            shader_program.delete_program();

            std::vector<Shader*> new_shaders;
            if (parent.dirty_shaders) {
                new_shaders.insert(new_shaders.end(), parent.shaders.begin(), parent.shaders.end());
            }

            if (dirty_shaders) {
                new_shaders.insert(new_shaders.end(), shaders.begin(), shaders.end());
            }

            shader_program = ShaderProgram(new_shaders);
        }

        if (transform.is_dirty()) {
            glUniformMatrix4fv(shader_program.get_uniform_location(__UNIFORM_GROUP_TRANSFORM), 1, GL_TRUE,
                    transform.to_matrix().array);
            transform.clean();
        }

        if (parent.transform.is_dirty()) {
            glUniformMatrix4fv(shader_program.get_uniform_location(__UNIFORM_GROUP_TRANSFORM), 1, GL_TRUE,
                    transform.to_matrix().array);
        }
    }

    void RenderGroup::draw(void) {
        shader_program.use_program();
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
        remove_from_vector(children, &renderable);
        dirty_children = true;
    }

    RenderableFactory &RenderGroup::get_renderable_factory(void) const {
        return renderable_factory;
    }

}

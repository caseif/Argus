// module core
#include "internal/util.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    using glext::glBindBuffer;
    using glext::glBufferData;
    using glext::glGenBuffers;

    using glext::glBindVertexArray;
    using glext::glEnableVertexAttribArray;
    using glext::glGenVertexArrays;
    using glext::glVertexAttribPointer;

    RenderGroup::RenderGroup(RenderLayer &parent):
            parent(parent),
            renderable_factory(*new RenderableFactory(*this)) {
        //TODO
    }

    void RenderGroup::destroy(void) {
        remove_from_vector(parent.children, this);

        delete this;
    }

    void RenderGroup::update_buffer(void) {
        printf("update\n");
        // init vertex array
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // init vertex buffer
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // compute how many vertices will be in this buffer
        size_t vertex_count = 0;
        for (Renderable *const child : this->children) {
            vertex_count += child->get_vertex_count();
        }

        // allocate a new buffer
        glBufferData(GL_ARRAY_BUFFER, vertex_count * __VERTEX_LEN_WORDS__ * __VERTEX_WORD_LEN__, nullptr, GL_DYNAMIC_DRAW);

        // render each child to the new buffer
        size_t offset = 0;
        for (Renderable *const child : this->children) {
            child->render(vbo, offset);
            offset += child->get_vertex_count();
        }

        //TODO: set attributes

        dirty = false;
    }

    void RenderGroup::add_renderable(Renderable &renderable) {
        children.insert(children.cbegin(), &renderable);
        dirty = true;
    }

    void RenderGroup::remove_renderable(Renderable &renderable) {
        remove_from_vector(children, &renderable);
        dirty = true;
    }

    RenderableFactory &RenderGroup::get_renderable_factory(void) const {
        return renderable_factory;
    }

}

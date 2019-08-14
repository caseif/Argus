#include "argus/renderer.hpp"
#include "internal/defines.hpp"
#include "internal/glext.hpp"

#include "vmmlib/vector.hpp"

#include <SDL2/SDL_opengl.h>

#define __TRIANGLE_VERTICES 3

namespace argus {

    using vmml::vec2f;

    using glext::glBufferSubData;

    RenderableTriangle::RenderableTriangle(RenderGroup &parent, vec2f corner_1, vec2f corner_2, vec2f corner_3):
            Renderable(parent) {
        this->corner_1 = corner_1;
        this->corner_2 = corner_2;
        this->corner_3 = corner_3;
    }

    void RenderableTriangle::render(const GLuint vbo, const size_t offset) const {
        float buffer_data[__TRIANGLE_VERTICES * __VERTEX_LEN * __VERTEX_WORD_LEN];
        //TODO

        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(buffer_data), buffer_data);
    }

    const unsigned int RenderableTriangle::get_vertex_count(void) const {
        return __TRIANGLE_VERTICES;
    }

}

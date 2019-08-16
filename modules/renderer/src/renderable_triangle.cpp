#include "argus/renderer.hpp"
#include "internal/defines.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

#define __TRIANGLE_VERTICES 3

namespace argus {

    using vmml::vec2f;

    using glext::glBufferSubData;

    RenderableTriangle::RenderableTriangle(RenderGroup &parent, vec2f const &corner_1, vec2f const &corner_2, vec2f const &corner_3):
            Renderable(parent),
            corner_1(corner_1),
            corner_2(corner_2),
            corner_3(corner_3) {
    }

    void RenderableTriangle::render(const GLuint vbo, const size_t offset) const {
        float buffer_data[__TRIANGLE_VERTICES * __VERTEX_LEN];
        buffer_data[0] = corner_1.x();
        buffer_data[1] = corner_1.y();
        buffer_data[9] = corner_2.x();
        buffer_data[10] = corner_2.y();
        buffer_data[18] = corner_3.x();
        buffer_data[19] = corner_3.y();

        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 4; j++) {
                buffer_data[i * __VERTEX_LEN + 2 + j] = 1.0;
            }
            for (size_t j = 0; j < 3; j++) {
                buffer_data[i * __VERTEX_LEN + 5 + j] = 1.0;
            }
        }

        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(buffer_data), buffer_data);
    }

    const unsigned int RenderableTriangle::get_vertex_count(void) const {
        return __TRIANGLE_VERTICES;
    }

}

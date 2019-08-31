#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

#define _SQUARE_VERTICES 6

namespace argus {

    using glext::glBufferSubData;

    RenderableSquare::RenderableSquare(RenderGroup &parent, Vertex const &corner_1, Vertex const &corner_2,
            Vertex const &corner_3, Vertex const &corner_4):
            Renderable(parent),
            corner_1(corner_1),
            corner_2(corner_2),
            corner_3(corner_3),
            corner_4(corner_4) {
    }

    void RenderableSquare::populate_buffer(void) {
        allocate_buffer(_SQUARE_VERTICES);

        buffer_vertex(corner_1);
        buffer_vertex(corner_2);
        buffer_vertex(corner_3);
        buffer_vertex(corner_1);
        buffer_vertex(corner_3);
        buffer_vertex(corner_4);
    }

    const unsigned int RenderableSquare::get_vertex_count(void) const {
        return _SQUARE_VERTICES;
    }

}

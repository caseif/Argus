#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

#define _TRIANGLE_VERTICES 3

namespace argus {

    using glext::glBufferSubData;

    RenderableTriangle::RenderableTriangle(RenderGroup &parent, Vertex const &corner_1, Vertex const &corner_2, Vertex const &corner_3):
            Renderable(parent),
            corner_1(corner_1),
            corner_2(corner_2),
            corner_3(corner_3) {
    }

    void RenderableTriangle::populate_buffer(void) {
        allocate_buffer(_TRIANGLE_VERTICES);

        buffer_vertex(corner_1);
        buffer_vertex(corner_2);
        buffer_vertex(corner_3);
    }

    const unsigned int RenderableTriangle::get_vertex_count(void) const {
        return _TRIANGLE_VERTICES;
    }

}

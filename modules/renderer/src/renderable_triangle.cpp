/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

#define _TRIANGLE_VERTICES 3

namespace argus {

    using glext::glBufferSubData;

    RenderableTriangle::RenderableTriangle(RenderGroup &parent, const Vertex &corner_1, const Vertex &corner_2, const Vertex &corner_3):
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

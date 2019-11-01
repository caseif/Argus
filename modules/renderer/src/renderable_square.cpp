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

#define _SQUARE_VERTICES 6

namespace argus {

    using glext::glBufferSubData;

    RenderableSquare::RenderableSquare(RenderGroup &parent, const Vertex &corner_1, const Vertex &corner_2,
            const Vertex &corner_3, const Vertex &corner_4):
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

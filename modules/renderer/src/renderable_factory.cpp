/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module renderer
#include "argus/renderer.hpp"

// module lowlevel
#include "argus/memory.hpp"

namespace argus {

    AllocPool g_triangle_alloc_pool(sizeof(RenderableTriangle), 128);
    AllocPool g_square_alloc_pool(sizeof(RenderableSquare), 128);

    RenderableFactory::RenderableFactory(RenderGroup &parent):
            parent(parent) {
    }

    RenderableTriangle &RenderableFactory::create_triangle(const Vertex &corner_1, const Vertex &corner_2, const Vertex &corner_3) const {
        return g_triangle_alloc_pool.construct<RenderableTriangle>(parent, corner_1, corner_2, corner_3);
    }

    RenderableSquare &RenderableFactory::create_square(const Vertex &corner_1, const Vertex &corner_2,
            const Vertex &corner_3, const Vertex &corner_4) const {
        //return *new RenderableSquare(parent, corner_1, corner_2, corner_3, corner_4);
        return g_triangle_alloc_pool.construct<RenderableSquare>(parent, corner_1, corner_2, corner_3, corner_4);
    }

}

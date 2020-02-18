/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module render
#include "argus/render/render_group.hpp"
#include "argus/render/renderable_factory.hpp"
#include "argus/render/renderable_square.hpp"
#include "argus/render/renderable_triangle.hpp"
#include "argus/render/transform.hpp"

// module lowlevel
#include "argus/memory.hpp"

namespace argus {

    //TODO: 512 seems pretty reasonable, I think
    AllocPool g_triangle_alloc_pool(sizeof(RenderableTriangle));
    AllocPool g_square_alloc_pool(sizeof(RenderableSquare));

    RenderableFactory::RenderableFactory(RenderGroup &parent):
            parent(parent) {
    }

    RenderableTriangle &RenderableFactory::create_triangle(const Vertex &corner_1, const Vertex &corner_2, const Vertex &corner_3) const {
        RenderableTriangle &shape = g_triangle_alloc_pool.construct<RenderableTriangle>(parent, corner_1, corner_2, corner_3);
        return shape;
    }

    RenderableSquare &RenderableFactory::create_square(const Vertex &corner_1, const Vertex &corner_2,
            const Vertex &corner_3, const Vertex &corner_4) const {
        //return *new RenderableSquare(parent, corner_1, corner_2, corner_3, corner_4);
        return g_square_alloc_pool.construct<RenderableSquare>(parent, corner_1, corner_2, corner_3, corner_4);
    }

    

}
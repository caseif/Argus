/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/renderer.hpp"

namespace argus {

    RenderableFactory::RenderableFactory(RenderGroup &parent):
            parent(parent) {
    }

    RenderableTriangle &RenderableFactory::create_triangle(const Vertex &corner_1, const Vertex &corner_2, const Vertex &corner_3) const {
        return *new RenderableTriangle(parent, corner_1, corner_2, corner_3);
    }

    RenderableSquare &RenderableFactory::create_square(const Vertex &corner_1, const Vertex &corner_2,
            const Vertex &corner_3, const Vertex &corner_4) const {
        return *new RenderableSquare(parent, corner_1, corner_2, corner_3, corner_4);
    }

}

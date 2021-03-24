/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/common/vertex.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include <atomic>
#include <vector>

#include <cstddef>

namespace argus {
    struct pimpl_RenderPrim2D {
        /**
         * \brief The vertices comprising this RenderPrim.
         */
        const std::vector<Vertex2D> vertices;

        pimpl_RenderPrim2D(const std::vector<Vertex2D> &vertices):
                vertices(vertices) {
        }

        pimpl_RenderPrim2D(const pimpl_RenderPrim2D&) = default;

        pimpl_RenderPrim2D(pimpl_RenderPrim2D&&) = delete;
    };
}

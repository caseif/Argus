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
#include "argus/render/render_prim.hpp"

#include <atomic>
#include <vector>

#include <cstddef>

namespace argus {
    struct pimpl_RenderPrim {
        /**
         * \brief The vertices comprising this RenderPrim.
         */
        const std::vector<Vertex> vertices;

        pimpl_RenderPrim(const std::vector<Vertex> &vertices):
                vertices(vertices) {
        }

        pimpl_RenderPrim(const pimpl_RenderPrim&) = default;

        pimpl_RenderPrim(pimpl_RenderPrim&&) = delete;
    };
}

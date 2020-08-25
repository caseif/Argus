/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/renderer.hpp"
#include "internal/render/types.hpp"

#include <atomic>
#include <vector>

namespace argus {
    struct pimpl_Renderer {
        /**
         * \brief The Window which this Renderer is mapped to.
         */
        Window &window;
        /**
         * \brief The child \link RenderLayer RenderLayers \endlink of this
         *        Renderer.
         */
        std::vector<RenderLayer*> render_layers;
        /**
         * \brief Whether the render resolution has recently been updated.
         */
        std::atomic_bool dirty_resolution;

        pimpl_Renderer(Window &window):
                window(window) {
        }
    };
}

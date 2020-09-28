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
#include "internal/render/renderer_impl.hpp"
#include "internal/render/types.hpp"

#include <atomic>
#include <vector>

namespace argus {
    struct pimpl_Renderer {
        /**
         * \brief The graphics backend used by this renderer.
         */
        RenderBackend backend;
        /**
         * \brief The specific Renderer implementation used by this wrapper.
         */
        RendererImpl *impl;
        /**
         * \brief The Window which this Renderer is mapped to.
         */
        Window &window;
        /**
         * \brief The child \link RenderLayer RenderLayers \endlink of this
         *        Renderer.
         */
        std::vector<RenderLayer*> render_layers;

        pimpl_Renderer(Window &window, RendererImpl *impl):
                window(window),
                impl(impl) {
        }
    };
}

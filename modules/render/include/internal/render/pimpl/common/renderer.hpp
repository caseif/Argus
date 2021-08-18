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
#include "argus/render/common/renderer.hpp"
#include "internal/render/renderer_impl.hpp"

#include <atomic>
#include <vector>

namespace argus {
    struct pimpl_Renderer {
        /**
         * \brief The Window which this Renderer is mapped to.
         */
        Window &window;
        /**
         * \brief The child \link Scene Scenes \endlink of this
         *        Renderer.
         */
        std::vector<Scene*> scenes;

        pimpl_Renderer(Window &window):
                window(window) {
        }

        pimpl_Renderer(const pimpl_Renderer&) = delete;

        pimpl_Renderer(pimpl_Renderer&&) = delete;
    };
}

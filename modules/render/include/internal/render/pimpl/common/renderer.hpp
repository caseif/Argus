/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

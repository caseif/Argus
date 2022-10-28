/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "argus/render/common/canvas.hpp"

#include <atomic>
#include <functional>
#include <map>
#include <vector>

namespace argus {
    struct AttachedViewport {
        const Camera2D &camera;
        const Viewport2D viewport;

        AttachedViewport(const Camera2D &camera, const Viewport2D &viewport):
                camera(camera),
                viewport(viewport) {
        }
    };

    struct pimpl_Canvas {
        /**
         * \brief The Window which this Canvas is mapped to.
         */
        Window &window;

        std::map<std::string, AttachedViewport> viewports;

        pimpl_Canvas(Window &window):
                window(window) {
        }

        pimpl_Canvas(const pimpl_Canvas&) = delete;

        pimpl_Canvas(pimpl_Canvas&&) = delete;
    };
}

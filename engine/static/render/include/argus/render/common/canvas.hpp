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

#include "argus/lowlevel/time.hpp"

#include "argus/render/2d/camera_2d.hpp"
#include "argus/render/2d/attached_viewport_2d.hpp"
#include "argus/render/common/scene.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace argus {
    // forward declarations
    class Scene;
    class Scene2D;
    class TextureData;

    class Window;

    struct pimpl_Canvas;

    /**
     * \brief A construct which contains a set of Scenes which will be rendered
     *        to the screen by a renderer implementation.
     *
     * Each Canvas has a one-to-one mapping with a Window, and a one-to-many
     * mapping with one or more \link Scene Scenes \endlink.
     *
     * A Canvas is guaranteed to have at least one Scene, considered to
     * be the "base" scene.
     *
     * \sa Window
     */
    class Canvas {
        public:
            pimpl_Canvas *const pimpl;

            /**
             * \brief Constructs a new Canvas attached to the given Window.
             *
             * \param window The Window to attach the new Canvas to.
             */
            Canvas(Window &window);

            Canvas(Canvas &rhs) = delete;

            Canvas(Canvas &&rhs) = delete;

            ~Canvas(void);

            /**
             * \brief Gets the Window which owns this Canvas.
             *
             * \return The Window which owns this Canvas.
             */
            Window &get_window(void) const;

            std::vector<std::reference_wrapper<AttachedViewport2D>> get_viewports_2d(void) const;

            std::optional<std::reference_wrapper<AttachedViewport>> find_viewport(const std::string &id) const;

            AttachedViewport2D &attach_viewport_2d(const std::string &id, const Viewport &viewport, Camera2D &camera,
                    uint32_t z_index);

            AttachedViewport2D &attach_default_viewport_2d(const std::string &id, Camera2D &camera);

            void detach_viewport_2d(const std::string &id);
    };
}

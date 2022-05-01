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

#include "argus/render/common/scene.hpp"

namespace argus {
    // forward declarations
    class Scene;
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
             * \brief Creates a new Scene with the given priority.
             *
             * Scenes with higher priority will be rendered after (ergo in front
             * of) those with lower priority.
             *
             * \param index The index of the new Scene. Higher-indexed
             *        Scenes are rendered atop lower-indexed ones.
             *
             * \return The created Scene.
             */
            Scene &create_scene(const SceneType type, const int index);

            /**
             * \brief Removes a Scene from this Canvas, destroying it in the
             *        process.
             *
             * \param scene The child Scene to remove.
             *
             * \throw std::invalid_argument If the supplied Scene is not owned
             *        by this Canvas.
             */
            void remove_scene(Scene &scene);
    };
}

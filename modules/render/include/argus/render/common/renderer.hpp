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

// module core
#include "argus/lowlevel/time.hpp"

// module render
#include "argus/render/common/scene.hpp"

namespace argus {
    // forward declarations
    class Scene;
    class TextureData;

    class Window;

    struct pimpl_Renderer;

    /**
     * \brief A construct which exposes functionality for rendering the entire
     *        screen space at once.
     *
     * Each Renderer has a one-to-one mapping with a Window, and a one-to-many
     * mapping with one or more \link Scene Scenes \endlink.
     *
     * A Renderer is guaranteed to have at least one Scene, considered to
     * be the "base" scene.
     *
     * \sa Window
     */
    class Renderer {
        public:
            pimpl_Renderer *const pimpl;

            /**
             * \brief Returns the Renderer associated with a given Window.
             *
             * \param window The Window to retrieve the Renderer for.
             *
             * \return The associated Renderer.
             */
            static Renderer &of_window(Window &window);

            /**
             * \brief Constructs a new Renderer attached to the given Window.
             *
             * \param window The Window to attach the new Renderer to.
             */
            Renderer(Window &window);

            Renderer(Renderer &rhs) = delete;

            Renderer(Renderer &&rhs) = delete;

            ~Renderer(void);

            Window &get_window() const;

            /**
             * \brief Initializes the Renderer.
             *
             * Initialization must be performed before render(TimeDelta) may be called.
             */
            void init(void);

            /**
             * \brief Outputs the Renderer's current state to the screen.
             *
             * \param delta The time in microseconds since the last frame.
             *
             * \remark This method accepts a TimeDelta to comply with the spec
             *         for engine callbacks as defined in the core module.
             */
            void render(const TimeDelta delta);

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
             * \brief Removes a Scene from this Renderer, destroying it in the
             *        process.
             *
             * \param scene The child Scene to remove.
             *
             * \throw std::invalid_argument If the supplied Scene is not owned
             *        by this Renderer.
             */
            void remove_scene(Scene &scene);
    };
}

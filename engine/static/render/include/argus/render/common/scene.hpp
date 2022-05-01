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

namespace argus {
    class Canvas;
    class Transform2D;

    struct pimpl_Scene;

    enum SceneType {
        TwoD,
        ThreeD,
    };

    /**
     * \brief Represents a Scene containing a set of geometry in space.
     *
     * Scenes are composited to the screen as stacked layers when a frame is
     * rendered.
     */
    class Scene {
        protected:
            /**
             * \brief Constructs a new Scene.
             *
             * \param type The type of Scene.
             */
            Scene(const SceneType type);

            Scene(const Scene &rhs) = delete;

            Scene(const Scene &&rhs) = delete;

        public:
            virtual ~Scene(void) = 0;

            const SceneType type;

            virtual pimpl_Scene *get_pimpl(void) const = 0;

            /**
             * \brief Gets the Canvas which is parent to this Scene.
             */
            const Canvas &get_canvas(void) const;

            /**
             * \brief Gets the Transform of this Scene.
             *
             * \return The Scene's Transform.
             */
            Transform2D &get_transform(void) const;

            /**
             * \brief Sets the Transform of this Scene.
             *
             * \param transform The new Transform.
             */
            void set_transform(const Transform2D &transform);
    };
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

namespace argus {
    class Renderer;
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
             * \brief Gets the Renderer which owns this Scene.
             */
            const Renderer &get_parent_renderer(void) const;

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
            void set_transform(Transform2D &transform);
    };
}

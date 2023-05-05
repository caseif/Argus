/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/atomic.hpp"

#include <optional>
#include <string>

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
        static std::optional<std::reference_wrapper<Scene>> find(const std::string &id);

        template<typename T>
        static std::optional<std::reference_wrapper<T>> find(const std::string &id) {
            //TODO: not sure if this is right
            return dynamic_cast<std::optional<std::reference_wrapper<T>>>(Scene::find(id));
        }

        virtual ~Scene(void) = 0;

        const SceneType type;

        virtual pimpl_Scene *get_pimpl(void) const = 0;
    };
}

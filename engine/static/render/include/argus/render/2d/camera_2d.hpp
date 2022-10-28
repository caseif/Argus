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

#include "argus/lowlevel/math.hpp"
#include <optional>
#include <string>

namespace argus {
    class Scene2D;
    class Transform2D;
    struct Viewport2D;

    struct pimpl_Camera2D;

    class Camera2D {
        public:
            pimpl_Camera2D *pimpl;

            Camera2D(const std::string &id, Scene2D &scene);

            Camera2D(const Camera2D&) = delete;

            Camera2D(Camera2D&&);

            ~Camera2D(void);

            const std::string &get_id(void) const;

            Scene2D &get_scene(void) const;

            Transform2D &get_transform(void) const;

            void set_transform(const Transform2D &transform);
    };
}

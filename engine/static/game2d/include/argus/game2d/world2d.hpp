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
#include <utility>

namespace argus {
    // forward declarations
    class Canvas;
    class AnimatedSprite;
    class Sprite;

    struct pimpl_World2D;

    class World2D {
        public:
            static World2D &create(const std::string &id, Canvas &canvas);

            static World2D &get(const std::string &id);

            pimpl_World2D *pimpl;

            World2D(const std::string &id, Canvas &canvas);

            World2D(World2D&) = delete;

            World2D(World2D&&) = delete;

            ~World2D(void);

            const std::string &get_id(void) const;

            Camera2D &get_camera(void) const;

            std::optional<std::reference_wrapper<Sprite>> get_sprite(const std::string &id) const;

            Sprite &add_sprite(const std::string &uid, const Vector2f &base_size, const std::string &texture_uid,
                    const std::pair<Vector2f, Vector2f> tex_coords);

            void remove_sprite(const std::string &id);

            std::optional<std::reference_wrapper<AnimatedSprite>> get_animated_sprite(const std::string &id) const;

            AnimatedSprite &add_animated_sprite(const std::string &id, const std::string &sprite_uid);

            void remove_animated_sprite(const std::string &id);
    };
}

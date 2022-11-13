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

#include "internal/game2d/animated_sprite.hpp"

#include <map>
#include <string>
#include <vector>

namespace argus {
    struct pimpl_AnimatedSprite {
        const Vector2f base_size;
        std::string def_anim;
        float speed;
        std::string def_atlas;
        const Vector2f tile_size;

        std::map<std::string, SpriteAnimation> animations;

        std::string cur_anim;

        bool paused;
        bool pending_reset;

        pimpl_AnimatedSprite(const Vector2f &base_size, float speed):
                base_size(base_size),
                speed(speed) {
        }
    };
}

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

#include "argus/lowlevel/math.hpp"

#include <map>

namespace argus {
    struct AnimationFrame {
        Vector2u offset;
        float duration;
    };

    struct SpriteAnimation {
        std::string id;

        bool loop;
        Padding padding;
        float def_duration;

        std::vector<AnimationFrame> frames;
    };

    struct SpriteDef {
        Vector2f base_size;
        std::string def_anim;
        float def_speed;
        std::string atlas;
        Vector2u tile_size;

        std::map<std::string, SpriteAnimation> animations;
    };
}

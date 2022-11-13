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


namespace argus {
    enum OffsetType {
        Tile,
        Absolute
    };

    struct SpriteAnimation {
        std::string id;

        std::string atlas;
        OffsetType offset_type;
        size_t frame_count;
        Vector2f tile_size;
        bool loop;
        Padding padding;
        float def_duration;
        float speed;
    };

    struct AnimationFrame {
        Vector2f offset;
        float duration;
    };
}

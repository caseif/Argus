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
#include "argus/lowlevel/time.hpp"

#include "argus/resman/resource.hpp"

#include "internal/game2d/animated_sprite.hpp"

#include <map>
#include <string>
#include <vector>

namespace argus {
    struct pimpl_AnimatedSprite {
        const std::string id;
        const Resource &def;

        std::map<std::string, size_t> anim_start_offsets;

        float speed;
        std::string cur_anim_id;
        SpriteAnimation *cur_anim;

        Dirtiable<size_t> cur_frame;
        Timestamp next_frame_update;
        bool paused;
        bool pending_reset;

        Transform2D transform;
        bool transform_dirty;

        pimpl_AnimatedSprite(const std::string &id, const Resource &def):
                id(id),
                def(def),
                speed(def.get<AnimatedSpriteDef>().def_speed) {
        }

        const AnimatedSpriteDef &get_def(void) const {
            return def.get<AnimatedSpriteDef>();
        }
    };
}

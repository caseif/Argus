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

#include <string>
#include <utility>

namespace argus {
    struct pimpl_Sprite {
        std::string id;
        Vector2f base_size;
        std::string texture_uid;
        std::pair<Vector2f, Vector2f> tex_coords;

        Transform2D transform;
        bool transform_dirty;

        pimpl_Sprite(const std::string &id, const Vector2f &base_size, const std::string &texture_uid,
                const std::pair<Vector2f, Vector2f> &tex_coords):
            id(id),
            base_size(base_size),
            texture_uid(texture_uid),
            tex_coords(tex_coords),
            transform_dirty(true) {
        }
    };
}

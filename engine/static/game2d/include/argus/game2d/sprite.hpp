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

#include "argus/render/common/transform.hpp"

#include <string>
#include <utility>

namespace argus {
    // forward declarations
    struct pimpl_Sprite;

    class Sprite {
       public:
        pimpl_Sprite *pimpl;

        Sprite(const std::string &id, const Vector2f &base_size, const std::string &texture_uid,
                const std::pair<Vector2f, Vector2f> tex_coords);

        Sprite(Sprite&) = delete;

        Sprite(Sprite&&);

        ~Sprite(void);

        const std::string &get_id(void) const;

        const Vector2f &get_base_size(void) const;

        const std::string &get_texture(void) const;

        const std::pair<Vector2f, Vector2f> &get_texture_coords(void) const;

        const Transform2D &get_transform(void) const;

        void set_transform(const Transform2D &transform);
    };
}

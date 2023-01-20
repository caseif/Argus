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

#include "argus/render/common/transform.hpp"

#include "argus/game2d/sprite.hpp"

#include <string>

namespace argus {
    // forward declarations
    struct pimpl_StaticObject2D;

    class StaticObject2D {
       public:
        pimpl_StaticObject2D *pimpl;

        StaticObject2D(const std::string &id, const std::string &sprite_uid,
                const Vector2f &size, const Transform2D &transform);

        StaticObject2D(StaticObject2D&) = delete;

        StaticObject2D(StaticObject2D &&rhs) noexcept;

        ~StaticObject2D(void);

        const std::string &get_id(void) const;

        const Vector2f &get_size(void) const;

        const Transform2D &get_transform(void) const;

        Sprite &get_sprite(void) const;
    };
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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
#include "argus/render/2d/render_object_2d.hpp"

#include <optional>
#include <string>
#include <utility>

namespace argus {
    struct pimpl_StaticObject2D {
        const Handle handle;
        Resource &sprite_def_res;
        Sprite &sprite;
        const Vector2f size;
        const uint32_t z_index;
        const bool can_occlude_light;
        const Transform2D transform;

        std::optional<Handle> render_obj;

        pimpl_StaticObject2D(Handle handle, Resource &sprite_def_res, Sprite &sprite,
                const Vector2f &size, uint32_t z_index, bool can_occlude_light,
                Transform2D transform):
            handle(handle),
            sprite_def_res(sprite_def_res),
            sprite(sprite),
            size(size),
            z_index(z_index),
            can_occlude_light(can_occlude_light),
            transform(std::move(transform)),
            render_obj() {
        }
    };
}

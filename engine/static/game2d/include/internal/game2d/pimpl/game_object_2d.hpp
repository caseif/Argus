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
#include "argus/render/2d/render_object_2d.hpp"

#include "argus/game2d/sprite.hpp"

namespace argus {
    struct pimpl_GameObject2D {
        const Uuid uuid;
        const Vector2f size;
        Dirtiable<Transform2D> transform;

        Resource &sprite_def_res;
        Sprite &sprite;

        std::string render_obj;

        pimpl_GameObject2D(Uuid uuid, const Vector2f &size, const Transform2D &transform,
                Resource &sprite_def_res, Sprite &sprite):
            uuid(uuid),
            size(size),
            transform(transform),
            sprite_def_res(sprite_def_res),
            sprite(sprite),
            render_obj("") {
        }
    };
}

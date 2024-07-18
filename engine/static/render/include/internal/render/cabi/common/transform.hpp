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

#include "argus/render/cabi/common/transform.h"

#include "argus/render/common/transform.hpp"

#include "argus/lowlevel/cabi/math/vector.h"

static inline ArgusTransform2d wrap_transform_2d(const argus::Transform2D &transform) {
    auto translation = transform.get_translation();
    auto scale = transform.get_scale();
    ArgusTransform2d wrapped_transform {
            *reinterpret_cast<argus_vector_2f_t *>(&translation),
            *reinterpret_cast<argus_vector_2f_t *>(&scale),
            transform.get_rotation(),
    };
    return wrapped_transform;
}

static inline argus::Transform2D unwrap_transform_2d(const ArgusTransform2d &transform) {
    return argus::Transform2D(
            *reinterpret_cast<const argus::Vector2f *>(&transform.translation),
            transform.rotation,
            *reinterpret_cast<const argus::Vector2f *>(&transform.scale)
    );
}

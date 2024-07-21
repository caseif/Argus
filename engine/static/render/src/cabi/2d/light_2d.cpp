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

#include "argus/render/cabi/2d/light_2d.h"
#include "internal/render/cabi/common/transform.hpp"

#include "argus/render/2d/light_2d.hpp"

#include "argus/lowlevel/cabi/handle.h"
#include "argus/lowlevel/cabi/math/vector.h"
#include "internal/lowlevel/cabi/handle.hpp"

#include "argus/lowlevel/math/vector.hpp"

#ifdef __is_layout_compatible
static_assert(__is_layout_compatible(ArgusLight2dParameters, argus::ArgusLight2dParameters));
#endif

static inline argus::Light2D &_as_ref(argus_light_2d_t light) {
    return *reinterpret_cast<argus::Light2D *>(light);
}

static inline const argus::Light2D &_as_ref(argus_light_2d_const_t light) {
    return *reinterpret_cast<const argus::Light2D *>(light);
}

extern "C" {

ArgusHandle argus_light_2d_get_handle(argus_light_2d_const_t light) {
    return wrap_handle(_as_ref(light).get_handle());
}

ArgusLight2dType argus_light_2d_get_type(argus_light_2d_const_t light) {
    return ArgusLight2dType(_as_ref(light).get_type());
}

bool argus_light_2d_is_occludable(argus_light_2d_const_t light) {
    return _as_ref(light).is_occludable();
}

argus_vector_3f_t argus_light_2d_get_color(argus_light_2d_const_t light) {
    return *reinterpret_cast<const argus_vector_3f_t *>(&_as_ref(light).get_color());
}

void argus_light_2d_set_color(argus_light_2d_t light, argus_vector_3f_t color) {
    _as_ref(light).set_color(*reinterpret_cast<argus::Vector3f *>(&color));
}

ArgusLight2dParameters argus_light_2d_get_parameters(argus_light_2d_const_t light) {
    return *reinterpret_cast<const ArgusLight2dParameters *>(&_as_ref(light).get_parameters());
}

void argus_light_2d_set_parameters(argus_light_2d_t light, ArgusLight2dParameters params) {
    _as_ref(light).set_parameters(*reinterpret_cast<argus::LightParameters *>(&params));
}

ArgusTransform2d argus_light_2d_get_transform(argus_light_2d_const_t light) {
    return wrap_transform_2d(_as_ref(light).get_transform());
}

void argus_light_2d_set_transform(argus_light_2d_t light, ArgusTransform2d transform) {
    _as_ref(light).set_transform(unwrap_transform_2d(transform));
}

}

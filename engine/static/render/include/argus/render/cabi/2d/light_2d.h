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

#ifdef __cplusplus
extern "C" {
#endif

#include "argus/render/cabi/common/transform.h"

#include "argus/lowlevel/cabi/handle.h"
#include "argus/lowlevel/cabi/math/vector.h"

#include <stdbool.h>
#include <stdint.h>

typedef void *argus_light_2d_t;
typedef const void *argus_light_2d_const_t;

typedef enum Light2dType {
    Point = 0,
} Light2dType;

typedef struct LightParameters {
    float intensity;
    uint32_t falloff_gradient;
    float falloff_multiplier;
    float falloff_buffer;
    uint32_t shadow_falloff_gradient;
    float shadow_falloff_multiplier;
} LightParameters;

ArgusHandle argus_light_2d_get_handle(argus_light_2d_const_t light);

Light2dType argus_light_2d_get_type(argus_light_2d_const_t light);

bool argus_light_2d_is_occludable(argus_light_2d_const_t light);

argus_vector_3f_t argus_light_2d_get_color(argus_light_2d_const_t light);

void argus_light_2d_set_color(argus_light_2d_t light, argus_vector_3f_t color);

LightParameters argus_light_2d_get_parameters(argus_light_2d_const_t light);

void argus_light_2d_set_parameters(argus_light_2d_t light, LightParameters params);

ArgusTransform2d argus_light_2d_get_transform(argus_light_2d_const_t light);

void set_transform(argus_light_2d_t light, ArgusTransform2d transform);

#ifdef __cplusplus
}
#endif

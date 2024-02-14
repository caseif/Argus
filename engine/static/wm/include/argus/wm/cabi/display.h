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

#ifdef __cplusplus
extern "C" {
#endif

#include "argus/lowlevel/cabi/math.h"

#include <stddef.h>
#include <stdint.h>

typedef void *argus_display_t;
typedef const void *argus_display_const_t;

typedef struct argus_display_mode_t {
    argus_vector_2u_t resolution;
    uint16_t refresh_rate;
    argus_vector_4u_t color_depth;
    uint32_t extra_data;
} argus_display_mode_t;

void argus_display_get_available_displays(size_t *out_count, argus_display_const_t *out_displays);

const char *argus_display_get_name(argus_display_const_t self);

argus_vector_2i_t argus_display_get_position(argus_display_const_t self);

void argus_display_get_display_modes(argus_display_const_t self, size_t *out_count, argus_display_mode_t *out_modes);

#ifdef __cplusplus
}
#endif

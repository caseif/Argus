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

#include "argus/render/cabi/2d/render_group_2d.h"
#include "argus/render/cabi/2d/scene_2d.h"

#include "argus/lowlevel/cabi/handle.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void *argus_render_object_2d_t;
typedef const void *argus_render_object_2d_const_t;

ArgusHandle argus_render_object_2d_get_handle(argus_render_object_2d_const_t obj);

argus_scene_2d_const_t argus_render_object_2d_get_scene(argus_render_object_2d_const_t obj);

argus_render_group_2d_const_t argus_render_object_2d_get_parent(argus_render_object_2d_const_t obj);

const char *argus_render_object_2d_get_material(argus_render_object_2d_const_t obj);

size_t argus_render_object_2d_get_primitives_count(argus_render_object_2d_const_t obj);

//TODO: get_primitives (it requires heap allocation and isn't used outside this module)

argus_vector_2f_t argus_render_object_2d_get_anchor_point(argus_render_object_2d_const_t obj);

argus_vector_2f_t argus_render_object_2d_get_atlas_stride(argus_render_object_2d_const_t obj);

uint32_t argus_render_object_2d_get_z_index(argus_render_object_2d_const_t obj);

float argus_render_object_2d_get_light_opacity(argus_render_object_2d_const_t obj);

void argus_render_object_2d_set_light_opacity(argus_render_object_2d_t obj, float opacity);

argus_vector_2u_t argus_render_object_2d_get_active_frame(argus_render_object_2d_t obj, bool *out_dirty);

void argus_render_object_2d_set_active_frame(argus_render_object_2d_t obj, argus_vector_2u_t frame);

ArgusTransform2d argus_render_object_2d_peek_transform(argus_render_object_2d_const_t obj);

ArgusTransform2d argus_render_object_2d_get_transform(argus_render_object_2d_t obj);

void argus_render_object_2d_set_transform(argus_render_object_2d_t obj, ArgusTransform2d transform);

argus_render_object_2d_t argus_render_object_2d_copy(argus_render_object_2d_const_t obj,
        argus_render_group_2d_t parent);

#ifdef __cplusplus
}
#endif

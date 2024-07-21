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

#include "argus/render/cabi/2d/render_prim_2d.h"
#include "argus/render/cabi/common/transform.h"

#include "argus/lowlevel/cabi/handle.h"
#include "argus/lowlevel/cabi/math/vector.h"

#include <stddef.h>
#include <stdint.h>

typedef void *argus_render_group_2d_t;
typedef const void *argus_render_group_2d_const_t;

// forward declarations
typedef void *argus_render_object_2d_t;
typedef const void *argus_render_object_2d_const_t;

typedef void *argus_scene_2d_t;
typedef const void *argus_scene_2d_const_t;

ArgusHandle argus_render_group_2d_get_handle(argus_render_group_2d_const_t group);

argus_scene_2d_t argus_render_group_2d_get_scene(argus_render_group_2d_const_t group);

argus_render_group_2d_t argus_render_group_2d_get_parent(argus_render_group_2d_const_t group);

ArgusHandle argus_render_group_2d_add_group(argus_render_group_2d_t group, ArgusTransform2d transform);

ArgusHandle argus_render_group_2d_add_object(argus_render_group_2d_t group, const char *material,
        const ArgusRenderPrimitive2d *primitives, size_t primitives_count, argus_vector_2f_t anchor_point,
        argus_vector_2f_t atlas_stride, uint32_t z_index, float light_opacity, ArgusTransform2d transform);

void argus_render_group_2d_remove_group(argus_render_group_2d_t group, ArgusHandle handle);

void argus_render_group_2d_remove_object(argus_render_group_2d_t group, ArgusHandle handle);

ArgusTransform2d argus_render_group_2d_peek_transform(argus_render_group_2d_const_t group);

ArgusTransform2d argus_render_group_2d_get_transform(argus_render_group_2d_t group);

void argus_render_group_2d_set_transform(argus_render_group_2d_t group, ArgusTransform2d transform);

argus_render_group_2d_t argus_render_group_2d_copy(argus_render_group_2d_t group);

#ifdef __cplusplus
}
#endif

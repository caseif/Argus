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

#include "argus/render/cabi/2d/camera_2d.h"
#include "argus/render/cabi/2d/light_2d.h"
#include "argus/render/cabi/2d/render_group_2d.h"
#include "argus/render/cabi/2d/render_object_2d.h"
#include "argus/render/cabi/2d/render_prim_2d.h"
#include "argus/render/cabi/2d/scene_2d.h"
#include "argus/render/cabi/common/transform.h"

#include "argus/lowlevel/cabi/handle.h"
#include "argus/lowlevel/cabi/math/vector.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void *argus_scene_2d_t;
typedef const void *argus_scene_2d_const_t;

argus_scene_2d_t argus_scene_2d_create(const char *id);

const char *argus_scene_2d_get_id(argus_scene_2d_const_t scene);

bool argus_scene_2d_is_lighting_enabled(argus_scene_2d_const_t scene);

void argus_scene_2d_set_lighting_enabled(argus_scene_2d_t scene, bool enabled);

float argus_scene_2d_peek_ambient_light_level(argus_scene_2d_const_t scene);

float argus_scene_2d_get_ambient_light_level(argus_scene_2d_t scene, bool *out_dirty);

void argus_scene_2d_set_ambient_light_level(argus_scene_2d_t scene, float level);

argus_vector_3f_t argus_scene_2d_peek_ambient_light_color(argus_scene_2d_const_t scene);

argus_vector_3f_t argus_scene_2d_get_ambient_light_color(argus_scene_2d_t scene, bool *out_dirty);

void argus_scene_2d_set_ambient_light_color(argus_scene_2d_t scene, argus_vector_3f_t color);

size_t argus_scene_2d_get_lights_count(argus_scene_2d_t scene);

void argus_scene_2d_get_lights(argus_scene_2d_t scene, argus_light_2d_t *dest, size_t count);

size_t argus_scene_2d_get_lights_count_for_render(argus_scene_2d_t scene);

void argus_scene_2d_get_lights_for_render(argus_scene_2d_t scene, argus_light_2d_const_t *dest, size_t count);

ArgusHandle argus_scene_2d_add_light(argus_scene_2d_t scene, ArgusLight2dType type, bool is_occludable,
        argus_vector_3f_t color, ArgusLight2dParameters params, ArgusTransform2d initial_transform);

argus_light_2d_t argus_scene_2d_get_light(argus_scene_2d_t scene, ArgusHandle handle);

void argus_scene_2d_remove_light(argus_scene_2d_t scene, ArgusHandle handle);

argus_render_group_2d_t argus_scene_2d_get_group(argus_scene_2d_t scene, ArgusHandle handle);

argus_render_object_2d_t argus_scene_2d_get_object(argus_scene_2d_t scene, ArgusHandle handle);

ArgusHandle argus_scene_2d_add_group(argus_scene_2d_t scene, ArgusTransform2d transform);

ArgusHandle argus_scene_2d_add_object(argus_scene_2d_t scene, const char *material, const ArgusRenderPrimitive2d *primitives,
        size_t primitives_count, argus_vector_2f_t anchor_point, argus_vector_2f_t atlas_stride, uint32_t z_index,
        float light_opacity, ArgusTransform2d transform);

void argus_scene_2d_remove_group(argus_scene_2d_t scene, ArgusHandle handle);

void argus_scene_2d_remove_object(argus_scene_2d_t scene, ArgusHandle handle);

argus_camera_2d_t argus_scene_2d_find_camera(argus_scene_2d_const_t scene, const char *id);

argus_camera_2d_t argus_scene_2d_create_camera(argus_scene_2d_t scene, const char *id);

void argus_scene_2d_destroy_camera(argus_scene_2d_t scene, const char *id);

void argus_scene_2d_lock_render_state(argus_scene_2d_t scene);

void argus_scene_2d_unlock_render_state(argus_scene_2d_t scene);

#ifdef __cplusplus
}
#endif

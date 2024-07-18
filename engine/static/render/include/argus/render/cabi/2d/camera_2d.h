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

#include <stdbool.h>

typedef void *argus_camera_2d_t;
typedef const void *argus_camera_2d_const_t;

// forward declarations
typedef void *argus_scene_2d_t;
typedef const void *argus_scene_2d_const_t;

const char *argus_camera_2d_get_id(argus_camera_2d_const_t camera);

argus_scene_2d_t argus_camera_2d_get_scene(argus_camera_2d_const_t camera);

ArgusTransform2d argus_camera_2d_peek_transform(argus_camera_2d_const_t camera);

ArgusTransform2d argus_camera_2d_get_transform(argus_camera_2d_t camera, bool *out_dirty);

void argus_camera_2d_set_transform(argus_camera_2d_t camera, ArgusTransform2d transform);

#ifdef __cplusplus
}
#endif

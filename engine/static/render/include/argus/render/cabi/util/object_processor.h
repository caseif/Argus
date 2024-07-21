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

#include "argus/render/cabi/2d/render_object_2d.h"
#include "argus/render/cabi/2d/scene_2d.h"
#include "argus/render/cabi/common/transform.h"

#include "argus/resman/cabi/resource.h"

#include "argus/lowlevel/cabi/handle.h"

#include <stdbool.h>
#include <stddef.h>

typedef void *argus_processed_render_object_2d_t;

typedef argus_processed_render_object_2d_t(*argus_process_render_obj_2d_fn_t)(argus_render_object_2d_const_t obj,
        argus_matrix_4x4_t transform, void *extra);

typedef void(*argus_update_render_obj_2d_fn_t)(argus_render_object_2d_const_t obj,
        argus_processed_render_object_2d_t proc_obj, argus_matrix_4x4_t transform, bool is_transform_dirty,
        void *extra);

typedef struct ArgusProcessedObjectMap {
    size_t count;
    size_t capacity;
    ArgusHandle *keys;
    argus_processed_render_object_2d_t *values;
} ArgusProcessedObjectMap;

void argus_process_objects_2d(argus_scene_2d_const_t scene, ArgusProcessedObjectMap *obj_map,
        argus_process_render_obj_2d_fn_t process_new_fn, argus_update_render_obj_2d_fn_t update_fn, void *extra);

void argus_processed_object_map_free(ArgusProcessedObjectMap *map);

#ifdef __cplusplus
}
#endif

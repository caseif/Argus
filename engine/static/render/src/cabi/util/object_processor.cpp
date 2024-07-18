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

#include "argus/render/cabi/common/transform.h"
#include "argus/render/cabi/util/object_processor.h"

#include "internal/lowlevel/cabi/handle.hpp"

#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/util/object_processor.hpp"

#include "argus/resman/resource.hpp"

#include "argus/core/engine.hpp"

#include "argus/lowlevel/debug.hpp"

#include <algorithm>
#include <map>

#include <cstddef>
#include <cstdlib>
#include <cstring>

extern "C" {

void argus_process_objects_2d(argus_scene_2d_const_t scene, ArgusProcessedObjectMap *obj_map,
        argus_process_render_obj_2d_fn_t process_new_fn, argus_update_render_obj_2d_fn_t update_fn, void *extra) {
    std::map<argus::Handle, argus::ProcessedRenderObject2DPtr> unwrapped_map;

    for (size_t i = 0; i < obj_map->count; i++) {
        unwrapped_map.insert({ unwrap_handle(obj_map->keys[i]), obj_map->values[i] });
    }

    const auto &real_scene = *reinterpret_cast<const argus::Scene2D *>(scene);
    argus::process_objects_2d(real_scene, unwrapped_map,
            [process_new_fn](const argus::RenderObject2D &obj, const argus::Matrix4 &transform, void *extra) {
                argus_matrix_4x4_t mat;
                memcpy(mat.cells, transform.data, sizeof(mat.cells));
                return process_new_fn(&obj, mat, extra);
            },
            [update_fn](const argus::RenderObject2D &obj, argus_processed_render_object_2d_t proc_obj,
                    const argus::Matrix4 &transform, bool is_transform_dirty, void *extra) -> void * {
                argus_matrix_4x4_t mat;
                memcpy(mat.cells, transform.data, sizeof(mat.cells));
                return update_fn(&obj, proc_obj, mat, is_transform_dirty, extra);
            },
            extra);

    if (obj_map->keys == nullptr) {
        obj_map->keys = reinterpret_cast<ArgusHandle *>(
                malloc(sizeof(ArgusHandle) * unwrapped_map.size()));
        obj_map->values = reinterpret_cast<argus_processed_render_object_2d_t *>(
                malloc(sizeof(argus_processed_render_object_2d_t) * unwrapped_map.size()));
        obj_map->capacity = unwrapped_map.size();
    }
    if (unwrapped_map.size() > obj_map->capacity) {
        obj_map->keys = reinterpret_cast<ArgusHandle *>(
                realloc(obj_map->keys, sizeof(ArgusHandle) * unwrapped_map.size()));
        obj_map->values = reinterpret_cast<argus_processed_render_object_2d_t *>(
                realloc(obj_map->values, sizeof(argus_processed_render_object_2d_t) * unwrapped_map.size()));
        obj_map->capacity = unwrapped_map.size();
    }
    obj_map->count = unwrapped_map.size();

    size_t i = 0;
    for (const auto &[key, val] : unwrapped_map) {
        obj_map->keys[i] = wrap_handle(key);
        obj_map->values[i] = val;
        i++;
    }
}

void argus_processed_object_map_free(ArgusProcessedObjectMap *map) {
    if (map->keys != nullptr) {
        free(map->keys);
    }

    if (map->values != nullptr) {
        free(map->values);
    }
}

}

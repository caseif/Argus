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

#include "argus/lowlevel/vector.hpp"

#include "argus/render/util/object_processor.hpp"
#include "argus/render/2d/scene_2d.hpp"

#include "internal/render_vulkan/renderer/shader_mgmt.hpp"
#include "internal/render_vulkan/renderer/2d/object_proc_impl.hpp"
#include "internal/render_vulkan/renderer/2d/scene_compiler.hpp"
#include "internal/render_vulkan/state/processed_render_object.hpp"
#include "internal/render_vulkan/state/render_bucket.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"

namespace argus {
    static BucketKey _get_bucket_key(ProcessedRenderObject &processed_obj) {
        return BucketKey { processed_obj.material_res.uid, processed_obj.atlas_stride, processed_obj.z_index };
    }

    static void _handle_new_obj(Scene2DState &scene_state, ProcessedRenderObject &processed_obj) {
        RenderBucket *bucket;
        auto key = _get_bucket_key(processed_obj);
        auto existing_bucket_it = scene_state.render_buckets.find(key);
        if (existing_bucket_it != scene_state.render_buckets.end()) {
            bucket = existing_bucket_it->second;
        } else {
            bucket = &RenderBucket::create(processed_obj.material_res, processed_obj.atlas_stride,
                    processed_obj.z_index);
            scene_state.render_buckets[key] = bucket;
        }

        bucket->objects.push_back(&processed_obj);
        bucket->needs_rebuild = true;

        processed_obj.newly_created = false;
    }

    static void _handle_stale_obj(Scene2DState &scene_state, ProcessedRenderObject &processed_obj) {
        auto &state = scene_state.parent_state;

        deinit_object_2d(state, processed_obj);

        // we need to remove it from its containing bucket and flag the bucket for a rebuild
        auto *bucket = scene_state.render_buckets[_get_bucket_key(processed_obj)];
        remove_from_vector(bucket->objects, &processed_obj);
        bucket->needs_rebuild = true;

        processed_obj.destroy();
    }

    void compile_scene_2d(const Scene2D &scene, Scene2DState &scene_state) {
        process_objects_2d(scene, scene_state.processed_objs, create_processed_object_2d, update_processed_object_2d,
                &scene_state);

        for (auto it = scene_state.processed_objs.begin(); it != scene_state.processed_objs.end();) {
            auto &processed_obj = *reinterpret_cast<ProcessedRenderObject *>(it->second);
            if (processed_obj.newly_created) {
                _handle_new_obj(scene_state, processed_obj);
            } else if (!processed_obj.visited) {
                // wasn't visited this iteration, must not be present in the scene graph anymore
                _handle_stale_obj(scene_state, processed_obj);
                it = scene_state.processed_objs.erase(it);
                continue;
            }

            processed_obj.visited = false;
            ++it;
        }
    }
}
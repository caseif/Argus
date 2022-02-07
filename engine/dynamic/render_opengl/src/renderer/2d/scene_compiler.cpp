/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include "internal/core/core_util.hpp"

#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/util/object_processor.hpp"

#include "internal/render_opengl/renderer/2d/object_proc_impl.hpp"
#include "internal/render_opengl/renderer/2d/scene_compiler.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/scene_state.hpp"

namespace argus {
    static void _handle_new_obj(Scene2DState &scene_state, ProcessedRenderObject &processed_obj) {
        RenderBucket *bucket;
        auto existing_bucket_it = scene_state.render_buckets.find(processed_obj.material_res.uid);
        if (existing_bucket_it != scene_state.render_buckets.end()) {
            bucket = existing_bucket_it->second;
        } else {
            bucket = &RenderBucket::create(processed_obj.material_res);
            scene_state.render_buckets[processed_obj.material_res.uid] = bucket;
        }

        bucket->objects.push_back(&processed_obj);
        bucket->needs_rebuild = true;
    }

    static void _handle_stale_obj(Scene2DState &scene_state, ProcessedRenderObject &processed_obj) {
        // wasn't visited this iteration, must not be present in the scene graph anymore

        deinit_object_2d(processed_obj);

        // we need to remove it from its containing bucket and flag the bucket for a rebuild
        auto *bucket = scene_state.render_buckets[processed_obj.material_res.uid];
        remove_from_vector(bucket->objects, &processed_obj);
        bucket->needs_rebuild = true;

        processed_obj.~ProcessedRenderObject();
    }

    void compile_scene_2d(const Scene2D &scene, Scene2DState &scene_state) {
        process_objects_2d(scene, scene_state.processed_objs, create_processed_object_2d, update_processed_object_2d);

        for (auto it = scene_state.processed_objs.begin(); it != scene_state.processed_objs.end();) {
            auto &processed_obj = *reinterpret_cast<ProcessedRenderObject*>(it->second);
            if (processed_obj.newly_created) {
                _handle_new_obj(scene_state, processed_obj);
            } else if (!processed_obj.visited) {
                _handle_stale_obj(scene_state, processed_obj);
                it = scene_state.processed_objs.erase(it);
                continue;
            }

            processed_obj.visited = false;
            ++it;
        }
    }
}
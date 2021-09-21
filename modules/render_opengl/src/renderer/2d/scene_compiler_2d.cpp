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

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "internal/core/core_util.hpp"

// module resman
#include "argus/resman/resource.hpp"

// module render
#include "argus/render/common/transform.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/pimpl/2d/render_group_2d.hpp"
#include "internal/render/pimpl/2d/scene_2d.hpp"

// module render_opengl
#include "internal/render_opengl/renderer/bucket_proc.hpp"
#include "internal/render_opengl/renderer/2d/object_proc.hpp"
#include "internal/render_opengl/renderer/2d/scene_compiler_2d.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/scene_state.hpp"

#include <atomic>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstring>

namespace argus {
    // forward declarations
    struct RendererState;

    static void _compute_abs_group_transform(const RenderGroup2D &group, Matrix4 &target) {
        group.get_transform().copy_matrix(target);
        const RenderGroup2D *cur = nullptr;
        const RenderGroup2D *parent = group.get_parent_group();

        while (parent != nullptr) {
            cur = parent;
            parent = parent->get_parent_group();

            Matrix4 new_transform;

            multiply_matrices(cur->get_transform().as_matrix(), target, new_transform);

            target = new_transform;
        }
    }

    static void _process_render_group_2d(RendererState &state, Scene2DState &scene_state, const RenderGroup2D &group,
            const bool recompute_transform, const Matrix4 running_transform) {
        bool new_recompute_transform = recompute_transform;
        Matrix4 cur_transform;

        if (recompute_transform) {
            // we already know we have to recompute the transform of this whole
            // branch since a parent was dirty
            multiply_matrices(running_transform, group.get_transform().as_matrix(), cur_transform);

            new_recompute_transform = true;
        } else if (group.get_transform().pimpl->dirty) {
            _compute_abs_group_transform(group, cur_transform);

            new_recompute_transform = true;

            group.get_transform().pimpl->dirty = false;
        }

        for (const RenderObject2D *child_object : group.pimpl->child_objects) {
            Matrix4 final_obj_transform;

            auto existing_it = scene_state.processed_objs.find(child_object);
            // if the object has already been processed previously
            if (existing_it != scene_state.processed_objs.end()) {
                // if a parent group or the object itself has had its transform updated
                existing_it->second->updated = new_recompute_transform || child_object->get_transform().pimpl->dirty;
                existing_it->second->visited = true;
            }

            if (new_recompute_transform) {
                multiply_matrices(cur_transform, child_object->get_transform().as_matrix(), final_obj_transform);
            } else if (child_object->get_transform().pimpl->dirty) {
                // parent transform hasn't been computed so we need to do it here
                Matrix4 group_abs_transform;
                _compute_abs_group_transform(group, group_abs_transform);

                multiply_matrices(group_abs_transform, child_object->get_transform().as_matrix(), final_obj_transform);
            } else {
                // nothing else to do if object and all parent groups aren't dirty
                return;
            }

            process_object_2d(scene_state, *child_object, final_obj_transform);
        }

        for (auto *child_group : group.pimpl->child_groups) {
            _process_render_group_2d(state, scene_state, *child_group, new_recompute_transform, cur_transform);
        }
    }

    static void _process_objects_2d(RendererState &state, Scene2DState &scene_state, const Scene2D &scene) {
        _process_render_group_2d(state, scene_state, scene.pimpl->root_group, false, {});

        for (auto it = scene_state.processed_objs.begin(); it != scene_state.processed_objs.end();) {
            auto *processed_obj = it->second;
            if (!processed_obj->visited) {
                // wasn't visited this iteration, must not be present in the scene graph anymore

                deinit_object_2d(*processed_obj);

                // we need to remove it from its containing bucket and flag the bucket for a rebuild
                auto *bucket = scene_state.render_buckets[it->second->material_res.uid];
                remove_from_vector(bucket->objects, processed_obj);
                bucket->needs_rebuild = true;

                processed_obj->~ProcessedRenderObject();

                it = scene_state.processed_objs.erase(it);

                continue;
            }

            it->second->visited = false;
            ++it;
        }
    }

    void compile_scene_2d(Scene2D &scene, RendererState &renderer_state, Scene2DState &scene_state) {
        _process_objects_2d(renderer_state, scene_state, scene);
        fill_buckets(scene_state);
    }
}

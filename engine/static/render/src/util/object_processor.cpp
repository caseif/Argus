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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/vector.hpp"

#include "argus/resman/resource.hpp"

#include "argus/render/common/transform.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/util/object_processor.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/pimpl/2d/render_group_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/scene_2d.hpp"

#include <atomic>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstring>

namespace argus {
    static void _compute_abs_group_transform(RenderGroup2D &group, Matrix4 &target) {
        group.get_transform()->copy_matrix(target);
        const RenderGroup2D *cur = nullptr;
        const RenderGroup2D *parent = group.get_parent_group();

        while (parent != nullptr) {
            cur = parent;
            parent = parent->get_parent_group();

            Matrix4 new_transform;

            multiply_matrices(cur->peek_transform().as_matrix(), target, new_transform);

            target = new_transform;
        }
    }

    static void _process_render_group_2d(ProcessedRenderObject2DMap &processed_obj_map, RenderGroup2D &group,
            const bool recompute_transform, const Matrix4 running_transform, ProcessRenderObj2DFn process_new_fn,
            UpdateRenderObj2DFn update_fn, void *extra) {
        bool new_recompute_transform = recompute_transform;
        Matrix4 cur_transform;

        auto group_transform = group.get_transform();
        if (recompute_transform) {
            // we already know we have to recompute the transform of this whole
            // branch since a parent was dirty
            multiply_matrices(running_transform, group_transform->as_matrix(), cur_transform);

            new_recompute_transform = true;
        } else if (group_transform.dirty) {
            _compute_abs_group_transform(group, cur_transform);

            new_recompute_transform = true;
        }

        for (RenderObject2D *child_object : group.pimpl->child_objects) {
            Matrix4 final_obj_transform;

            auto existing_it = processed_obj_map.find(child_object->pimpl->handle);

            auto obj_transform = child_object->get_transform();

            if (new_recompute_transform) {
                multiply_matrices(obj_transform->as_matrix(), cur_transform, final_obj_transform);
            } else if (obj_transform.dirty) {
                // parent transform hasn't been computed so we need to do it here
                Matrix4 group_abs_transform;
                _compute_abs_group_transform(group, group_abs_transform);

                multiply_matrices(obj_transform->as_matrix(), group_abs_transform, final_obj_transform);
            }
            // don't need to compute anything otherwise, update function will just mark the object as visited

            bool dirty_transform = new_recompute_transform || obj_transform.dirty;

            if (existing_it != processed_obj_map.end()) {
                update_fn(*child_object, existing_it->second, final_obj_transform, dirty_transform, extra);
            } else {
                auto *processed_obj = process_new_fn(*child_object, final_obj_transform, extra);

                processed_obj_map.insert({child_object->pimpl->handle, processed_obj});
            }
        }

        for (auto *child_group : group.pimpl->child_groups) {
            _process_render_group_2d(processed_obj_map, *child_group,
                    new_recompute_transform, cur_transform, process_new_fn, update_fn, extra);
        }
    }

    void process_objects_2d(const Scene2D &scene, ProcessedRenderObject2DMap &processed_obj_map,
            ProcessRenderObj2DFn process_new_fn, UpdateRenderObj2DFn update_fn, void *extra) {
        // need to acquire a lock to prevent buffer swapping while compiling scene
        scene.pimpl->read_lock.lock();
        _process_render_group_2d(processed_obj_map, *scene.pimpl->root_group_read, false, {},
                process_new_fn, update_fn, extra);
        scene.pimpl->read_lock.unlock();
    }
}

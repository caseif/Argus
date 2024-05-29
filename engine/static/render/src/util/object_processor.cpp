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

#include "argus/lowlevel/math.hpp"

#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/util/object_processor.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/pimpl/2d/render_group_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/scene_2d.hpp"

#include <map>

namespace argus {
    static void _compute_abs_group_transform(RenderGroup2D &group, Matrix4 &target) {
        group.get_transform().copy_matrix(target, {0, 0});
        const RenderGroup2D *cur = nullptr;
        const RenderGroup2D *parent = group.get_parent();

        while (parent != nullptr) {
            cur = parent;
            parent = parent->get_parent();

            Matrix4 new_transform = target * cur->peek_transform().as_matrix({0, 0});

            target = new_transform;
        }
    }

    static void _process_render_group_2d(ProcessedRenderObject2DMap &processed_obj_map, RenderGroup2D &group,
            const bool recompute_transform, const Matrix4 running_transform, std::map<Handle, uint16_t> &new_version_map,
            const ProcessRenderObj2DFn &process_new_fn, const UpdateRenderObj2DFn &update_fn, void *extra) {
        bool new_recompute_transform = recompute_transform;
        Matrix4 cur_transform;

        auto &scene = group.get_scene();
        auto cur_version_map = scene.pimpl->last_rendered_versions;

        new_version_map[group.get_handle()] = group.pimpl->version;

        auto group_transform = group.get_transform();
        if (recompute_transform) {
            // we already know we have to recompute the transform of this whole
            // branch since a parent was dirty
            cur_transform = group_transform.as_matrix({0, 0}) * running_transform;

            new_recompute_transform = true;
        } else if (group.pimpl->version != cur_version_map[group.get_handle()]) {
            _compute_abs_group_transform(group, cur_transform);

            new_recompute_transform = true;
        }

        for (RenderObject2D *child_object : group.pimpl->child_objects) {
            Matrix4 final_obj_transform;

            auto existing_it = processed_obj_map.find(child_object->get_handle());

            auto obj_transform = child_object->get_transform();

            auto obj_anchor = child_object->get_anchor_point();

            auto obj_dirty = child_object->pimpl->version != cur_version_map[child_object->get_handle()];

            new_version_map[child_object->get_handle()] = child_object->pimpl->version;

            if (new_recompute_transform) {
                final_obj_transform = cur_transform * obj_transform.as_matrix(obj_anchor);
            } else if (obj_dirty) {
                // parent transform hasn't been computed so we need to do it here
                Matrix4 group_abs_transform;
                _compute_abs_group_transform(group, group_abs_transform);

                final_obj_transform = group_abs_transform * obj_transform.as_matrix(obj_anchor);
            }
            // don't need to compute anything otherwise, update function will just mark the object as visited

            bool dirty_transform = new_recompute_transform || obj_dirty;

            if (existing_it != processed_obj_map.end()) {
                update_fn(*child_object, existing_it->second, final_obj_transform, dirty_transform, extra);
            } else {
                auto *processed_obj = process_new_fn(*child_object, final_obj_transform, extra);

                processed_obj_map.insert({child_object->get_handle(), processed_obj});
            }
        }

        for (auto *child_group : group.pimpl->child_groups) {
            _process_render_group_2d(processed_obj_map, *child_group,
                    new_recompute_transform, cur_transform, new_version_map, process_new_fn, update_fn, extra);
        }
    }

    void process_objects_2d(const Scene2D &scene, ProcessedRenderObject2DMap &processed_obj_map,
            const ProcessRenderObj2DFn &process_new_fn, const UpdateRenderObj2DFn &update_fn, void *extra) {
        std::map<Handle, uint16_t> new_version_map;

        // need to acquire a lock to prevent buffer swapping while compiling scene
        scene.pimpl->read_lock.lock();
        _process_render_group_2d(processed_obj_map, *scene.pimpl->root_group_read, false, {},
                new_version_map, process_new_fn, update_fn, extra);
        scene.pimpl->read_lock.unlock();

        // this map is internal to the renderer and thus doesn't need to be synchronized
        scene.pimpl->last_rendered_versions = new_version_map;
    }
}

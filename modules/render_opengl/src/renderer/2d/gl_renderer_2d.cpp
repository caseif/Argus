/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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
#include "argus/render/2d/render_layer_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/pimpl/2d/render_group_2d.hpp"
#include "internal/render/pimpl/2d/render_layer_2d.hpp"

// module render_opengl
#include "internal/render_opengl/renderer/bucket_proc.hpp"
#include "internal/render_opengl/renderer/2d/gl_renderer_2d.hpp"
#include "internal/render_opengl/renderer/2d/object_proc.hpp"
#include "internal/render_opengl/state/layer_state.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"

#include <atomic>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cstring>

namespace argus {
    // forward declarations
    struct RendererState;

    static void _compute_abs_group_transform(const RenderGroup2D &group, mat4_flat_t &target) {
        group.get_transform().copy_matrix(target);
        const RenderGroup2D *cur = nullptr;
        const RenderGroup2D *parent = group.get_parent_group();

        while (parent != nullptr) {
            cur = parent;
            parent = parent->get_parent_group();

            mat4_flat_t new_transform;

            multiply_matrices(cur->get_transform().as_matrix(), target, new_transform);

            memcpy(target, new_transform, 16 * sizeof(target[0]));
        }
    }

    static void _process_render_group_2d(RendererState &state, Layer2DState &layer_state, const RenderGroup2D &group,
            const bool recompute_transform, const mat4_flat_t running_transform) {
        bool new_recompute_transform = recompute_transform;
        mat4_flat_t cur_transform;

        if (recompute_transform) {
            // we already know we have to recompute the transform of this whole
            // branch since a parent was dirty
            _ARGUS_ASSERT(running_transform != nullptr, "running_transform is null\n");
            multiply_matrices(running_transform, group.get_transform().as_matrix(), cur_transform);

            new_recompute_transform = true;
        } else if (group.get_transform().pimpl->dirty) {
            _compute_abs_group_transform(group, cur_transform);

            new_recompute_transform = true;

            group.get_transform().pimpl->dirty = false;
        }

        for (const RenderObject2D *child_object : group.pimpl->child_objects) {
            mat4_flat_t final_obj_transform;

            auto existing_it = layer_state.processed_objs.find(child_object);
            // if the object has already been processed previously
            if (existing_it != layer_state.processed_objs.end()) {
                // if a parent group or the object itself has had its transform updated
                existing_it->second->updated = new_recompute_transform || child_object->get_transform().pimpl->dirty;
                existing_it->second->visited = true;
            }

            if (new_recompute_transform) {
                multiply_matrices(cur_transform, child_object->get_transform().as_matrix(), final_obj_transform);
            } else if (child_object->get_transform().pimpl->dirty) {
                // parent transform hasn't been computed so we need to do it here
                mat4_flat_t group_abs_transform;
                _compute_abs_group_transform(group, group_abs_transform);

                multiply_matrices(group_abs_transform, child_object->get_transform().as_matrix(), final_obj_transform);
            } else {
                // nothing else to do if object and all parent groups aren't dirty
                return;
            }

            process_object_2d(layer_state, *child_object, final_obj_transform);
        }

        for (auto *child_group : group.pimpl->child_groups) {
            _process_render_group_2d(state, layer_state, *child_group, new_recompute_transform, cur_transform);
        }
    }

    static void _process_objects_2d(RendererState &state, Layer2DState &layer_state,
            const RenderLayer2D &layer) {
        _process_render_group_2d(state, layer_state, layer.pimpl->root_group, false, nullptr);

        for (auto it = layer_state.processed_objs.begin(); it != layer_state.processed_objs.end();) {
            auto *processed_obj = it->second;
            if (!processed_obj->visited) {
                // wasn't visited this iteration, must not be present in the scene graph anymore

                deinit_object_2d(*processed_obj);

                // we need to remove it from its containing bucket and flag the bucket for a rebuild
                auto *bucket = layer_state.render_buckets[it->second->material_res.uid];
                remove_from_vector(bucket->objects, processed_obj);
                bucket->needs_rebuild = true;

                processed_obj->~ProcessedRenderObject();

                it = layer_state.processed_objs.erase(it);

                continue;
            }

            it->second->visited = false;
            ++it;
        }
    }

    void render_layer_2d(RenderLayer2D &layer, RendererState &renderer_state, Layer2DState &layer_state) {
        _process_objects_2d(renderer_state, layer_state, layer);
        fill_buckets(layer_state);
    }
}

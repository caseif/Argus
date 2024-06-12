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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/defines.hpp"
#include "argus/render/util/object_processor.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/renderer/2d/object_proc_impl.hpp"
#include "internal/render_vulkan/state/processed_render_object.hpp"
#include "internal/render_vulkan/state/render_bucket.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"
#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/util/memory.hpp"
#include "internal/render_vulkan/util/pipeline.hpp"

#include <map>

#include <climits>
#include <cstddef>

namespace argus {
    /*static size_t _count_vertices(const RenderObject2D &obj) {
        return std::accumulate(obj.get_primitives().cbegin(), obj.get_primitives().cend(), size_t(0),
                [](auto acc, const auto &prim) {
                    return acc + prim.get_vertex_count();
                }
        );
    }*/

    ProcessedRenderObject2DPtr create_processed_object_2d(const RenderObject2D &object,
            const Matrix4 &transform, void *scene_state_ptr) {
        auto &scene_state = *static_cast<SceneState *>(scene_state_ptr);
        auto &state = scene_state.parent_state;

        size_t vertex_count = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            vertex_count += prim.get_vertex_count();
        }

        // GCC complains (likely erroneously) about a dangling reference if the
        // Result is unwrapped inline, so we store it to a variable first
        auto mat_res_res = ResourceManager::instance().get_resource(object.get_material());
        const Resource &mat_res = mat_res_res
                .expect("Failed to load material " + object.get_material() + " for RenderObject2D");

        if (state.material_pipelines.find(object.get_material()) == state.material_pipelines.end()) {
            auto pipeline = create_pipeline(state, mat_res.get<Material>(), state.fb_render_pass);
            state.material_pipelines.insert({ object.get_material(), pipeline });
        }
        auto &pipeline = state.material_pipelines.find(object.get_material())->second;

        size_t vertex_comps = pipeline.vertex_len / sizeof(float);

        size_t buffer_size = vertex_count * vertex_comps * sizeof(float);

        affirm_precond(buffer_size <= INT_MAX, "Buffer size is too big");

        auto staging_buffer = alloc_buffer(state.device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                GraphicsMemoryPropCombos::DeviceRw);

        auto float_buffer = reinterpret_cast<float *>(staging_buffer.mapped);

        uint32_t total_vertices = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            for (auto &vertex : prim.get_vertices()) {
                size_t major_off = total_vertices * vertex_comps;
                size_t minor_off = 0;

                if (pipeline.reflection.has_attr(SHADER_ATTRIB_POSITION)) {
                    auto pos_vec = Vector4f(vertex.position.x, vertex.position.y, 0, 1);
                    auto transformed_pos = transform * pos_vec;
                    float_buffer[major_off + minor_off++] = transformed_pos.x;
                    float_buffer[major_off + minor_off++] = transformed_pos.y;
                }
                if (pipeline.reflection.has_attr(SHADER_ATTRIB_NORMAL)) {
                    float_buffer[major_off + minor_off++] = vertex.normal.x;
                    float_buffer[major_off + minor_off++] = vertex.normal.y;
                }
                if (pipeline.reflection.has_attr(SHADER_ATTRIB_COLOR)) {
                    float_buffer[major_off + minor_off++] = vertex.color.r;
                    float_buffer[major_off + minor_off++] = vertex.color.g;
                    float_buffer[major_off + minor_off++] = vertex.color.b;
                    float_buffer[major_off + minor_off++] = vertex.color.a;
                }
                if (pipeline.reflection.has_attr(SHADER_ATTRIB_TEXCOORD)) {
                    float_buffer[major_off + minor_off++] = vertex.tex_coord.x;
                    float_buffer[major_off + minor_off++] = vertex.tex_coord.y;
                }

                total_vertices += 1;
            }
        }

        auto &processed_obj = ProcessedRenderObject::create(mat_res, object.get_atlas_stride(),
                object.get_z_index(), object.get_light_opacity(), total_vertices);

        processed_obj.staging_buffer = staging_buffer;

        processed_obj.anim_frame = object.get_active_frame().value;

        processed_obj.visited = true;
        processed_obj.newly_created = true;

        return &processed_obj;
    }

    void update_processed_object_2d(const RenderObject2D &object, ProcessedRenderObject2DPtr proc_obj_ptr,
            const Matrix4 &transform, bool is_transform_dirty, void *scene_state_ptr) {
        auto &scene_state = *static_cast<SceneState *>(scene_state_ptr);
        auto &state = scene_state.parent_state;

        auto &proc_obj = *reinterpret_cast<ProcessedRenderObject *>(proc_obj_ptr);

        // if a parent group or the object itself has had its transform updated
        proc_obj.updated = is_transform_dirty;

        auto cur_frame = object.get_active_frame();
        if (cur_frame.dirty) {
            proc_obj.anim_frame = cur_frame.value;
            proc_obj.anim_frame_updated = true;
        }

        if (!is_transform_dirty) {
            // nothing to do
            proc_obj.visited = true;
            return;
        }

        // pipeline should be created by now
        auto &pipeline = state.material_pipelines.find(object.get_material())->second;

        size_t vertex_comps = (pipeline.reflection.has_attr(SHADER_ATTRIB_POSITION) ? SHADER_ATTRIB_POSITION_LEN : 0)
                + (pipeline.reflection.has_attr(SHADER_ATTRIB_NORMAL) ? SHADER_ATTRIB_NORMAL_LEN : 0)
                + (pipeline.reflection.has_attr(SHADER_ATTRIB_COLOR) ? SHADER_ATTRIB_COLOR_LEN : 0)
                + (pipeline.reflection.has_attr(SHADER_ATTRIB_TEXCOORD) ? SHADER_ATTRIB_TEXCOORD_LEN : 0);

        auto float_buffer = reinterpret_cast<float *>(proc_obj.staging_buffer.mapped);

        size_t total_vertices = 0;
        for (const RenderPrim2D &prim : object.get_primitives()) {
            for (auto &vertex : prim.get_vertices()) {
                size_t major_off = total_vertices * vertex_comps;
                size_t minor_off = 0;

                auto pos_vec = Vector4f(vertex.position.x, vertex.position.y, 0, 1);
                auto transformed_pos = transform * pos_vec;
                float_buffer[major_off + minor_off++] = transformed_pos.x;
                float_buffer[major_off + minor_off++] = transformed_pos.y;

                total_vertices += 1;
            }
        }

        proc_obj.visited = true;
        proc_obj.updated = true;
    }

    void deinit_object_2d(const RendererState &state, ProcessedRenderObject &obj) {
        UNUSED(state);
        unmap_buffer(obj.staging_buffer);
        free_buffer(obj.staging_buffer);
        obj.material_res.release();
    }
}

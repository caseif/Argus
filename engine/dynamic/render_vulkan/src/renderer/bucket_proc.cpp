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

#include "argus/lowlevel/debug.hpp"

#include "argus/render/defines.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/renderer/bucket_proc.hpp"
#include "internal/render_vulkan/state/render_bucket.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"
#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/util/command_buffer.hpp"

#include <climits>
#include <cstddef>
#include <cstdint>

namespace argus {
    void _try_free_buffer(const LogicalDevice &device, BufferInfo &buffer) {
        if (buffer.handle != nullptr) {
            free_buffer(device, buffer);
            buffer.handle = nullptr;
        }
    }

    void fill_buckets(SceneState &scene_state) {
        const auto &state = scene_state.parent_state;

        for (auto it = scene_state.render_buckets.begin(); it != scene_state.render_buckets.end();) {
            auto *bucket = it->second;

            if (bucket->ubo_buffer.handle == VK_NULL_HANDLE) {
                bucket->ubo_buffer = alloc_buffer(state.device, SHADER_UBO_OBJ_LEN,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                float uv_stride[] = { bucket->atlas_stride.x, bucket->atlas_stride.y };
                auto *mapped = map_buffer(state.device, bucket->ubo_buffer,
                        SHADER_UNIFORM_OBJ_UV_STRIDE_OFF, sizeof(uv_stride), 0);
                memcpy(mapped, uv_stride, sizeof(uv_stride));
                unmap_buffer(state.device, bucket->ubo_buffer);
            }

            if (bucket->objects.empty()) {
                _try_free_buffer(state.device, it->second->vertex_buffer);
                _try_free_buffer(state.device, it->second->anim_frame_buffer);
                _try_free_buffer(state.device, it->second->staging_vertex_buffer);
                _try_free_buffer(state.device, it->second->staging_anim_frame_buffer);
                _try_free_buffer(state.device, it->second->ubo_buffer);
                it->second->destroy();

                it = scene_state.render_buckets.erase(it);

                continue;
            }

            auto pipeline_it = scene_state.parent_state.material_pipelines.find(bucket->material_res.uid);
            affirm_precond(pipeline_it != scene_state.parent_state.material_pipelines.cend(),
                    "Cannot find material pipeline");

            // the pipeline should have been built during object processing
            auto &pipeline = scene_state.parent_state.material_pipelines.find(bucket->material_res.uid)->second;

            auto attr_position_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_POSITION);
            auto attr_normal_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_NORMAL);
            auto attr_color_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_COLOR);
            auto attr_texcoord_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_TEXCOORD);
            //auto attr_anim_frame_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_ANIM_FRAME);

            uint32_t vertex_comps = (attr_position_loc.has_value() ? SHADER_ATTRIB_POSITION_LEN : 0)
                                  + (attr_normal_loc.has_value() ? SHADER_ATTRIB_NORMAL_LEN : 0)
                                  + (attr_color_loc.has_value() ? SHADER_ATTRIB_COLOR_LEN : 0)
                                  + (attr_texcoord_loc.has_value() ? SHADER_ATTRIB_TEXCOORD_LEN : 0);

            size_t anim_frame_buf_len = 0;
            if (bucket->needs_rebuild) {
                size_t vertex_buf_len = 0;
                for (auto &obj : bucket->objects) {
                    vertex_buf_len += obj->staging_buffer.size;
                    anim_frame_buf_len += obj->vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN * sizeof(float);
                }

                _try_free_buffer(state.device, bucket->vertex_buffer);
                _try_free_buffer(state.device, bucket->anim_frame_buffer);
                _try_free_buffer(state.device, bucket->staging_vertex_buffer);
                _try_free_buffer(state.device, bucket->staging_anim_frame_buffer);

                affirm_precond(vertex_buf_len <= INT_MAX, "Buffer length is too big");

                bucket->vertex_buffer = alloc_buffer(state.device, vertex_buf_len,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                bucket->staging_vertex_buffer = alloc_buffer(state.device, vertex_buf_len,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                auto stride = vertex_comps * uint32_t(sizeof(float));
                affirm_precond(stride <= INT_MAX, "Vertex stride is too big");

                affirm_precond(anim_frame_buf_len <= INT_MAX, "Animation frame buffer length is too big");
                bucket->anim_frame_buffer = alloc_buffer(state.device, anim_frame_buf_len,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                bucket->staging_anim_frame_buffer = alloc_buffer(state.device, anim_frame_buf_len,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            } else {
                anim_frame_buf_len = bucket->vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN * sizeof(float);
            }

            bucket->vertex_count = 0;

            bool anim_buf_updated = false;

            size_t offset = 0;
            size_t anim_frame_off = 0;

            float *anim_frame_buf = static_cast<float *>(map_buffer(state.device, bucket->staging_anim_frame_buffer,
                    0, bucket->staging_anim_frame_buffer.size, 0));

            for (auto *processed : bucket->objects) {
                if (processed == nullptr) {
                    continue;
                }

                if (bucket->needs_rebuild || processed->updated) {
                    affirm_precond(offset <= INT_MAX, "Buffer offset is too big");
                    affirm_precond(processed->staging_buffer.size <= INT_MAX, "Buffer offset is too big");

                    auto cmd_buf = alloc_command_buffers(state.device, state.command_pool, 1).front();
                    copy_buffer(state.device, cmd_buf, processed->staging_buffer, 0, bucket->staging_vertex_buffer,
                            offset, processed->staging_buffer.size);
                    free_command_buffer(state.device, cmd_buf);
                }

                if (bucket->needs_rebuild || processed->anim_frame_updated) {
                    for (size_t i = 0; i < processed->vertex_count; i++) {
                        anim_frame_buf[anim_frame_off++] = float(processed->anim_frame.x);
                        anim_frame_buf[anim_frame_off++] = float(processed->anim_frame.y);
                    }
                    processed->anim_frame_updated = false;
                    anim_buf_updated = true;
                } else {
                    anim_frame_off += processed->vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN;
                }

                offset += processed->staging_buffer.size;

                bucket->vertex_count += processed->vertex_count;
            }

            unmap_buffer(state.device, bucket->staging_anim_frame_buffer);

            auto cmd_buf = alloc_command_buffers(state.device, state.command_pool, 1).front();
            copy_buffer(state.device, cmd_buf, bucket->staging_vertex_buffer, 0, bucket->vertex_buffer, 0,
                    bucket->staging_vertex_buffer.size);
            if (anim_buf_updated) {
                affirm_precond(anim_frame_buf_len <= INT_MAX, "Animated frame buffer length is too big");
                copy_buffer(state.device, cmd_buf, bucket->staging_anim_frame_buffer, 0, bucket->anim_frame_buffer, 0,
                        anim_frame_buf_len);
            }
            free_command_buffer(state.device, cmd_buf);

            bucket->needs_rebuild = false;

            it++;
        }
    }
}

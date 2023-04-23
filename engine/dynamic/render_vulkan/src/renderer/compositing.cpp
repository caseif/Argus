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

#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/material.hpp"
#include "argus/render/defines.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/renderer/compositing.hpp"
#include "internal/render_vulkan/renderer/shader_mgmt.hpp"
#include "internal/render_vulkan/state/render_bucket.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"
#include "internal/render_vulkan/state/viewport_state.hpp"
#include "internal/render_vulkan/util/command_buffer.hpp"
#include "internal/render_vulkan/util/descriptor_set.hpp"
#include "internal/render_vulkan/util/framebuffer.hpp"
#include "internal/render_vulkan/util/image.hpp"

#include "vulkan/vulkan.h"

#include <map>
#include <string>
#include <utility>

#include <climits>

#define BINDING_INDEX_VBO 0

namespace argus {
    struct TransformedViewport {
        int32_t top;
        int32_t bottom;
        int32_t left;
        int32_t right;
    };

    TransformedViewport _transform_viewport_to_pixels(const Viewport &viewport, const Vector2u &resolution) {
        float vp_h_scale;
        float vp_v_scale;
        float vp_h_off;
        float vp_v_off;

        auto min_dim = float(std::min(resolution.x, resolution.y));
        auto max_dim = float(std::max(resolution.x, resolution.y));
        switch (viewport.mode) {
            case ViewportCoordinateSpaceMode::Individual:
                vp_h_scale = float(resolution.x);
                vp_v_scale = float(resolution.y);
                vp_h_off = 0;
                vp_v_off = 0;
                break;
            case ViewportCoordinateSpaceMode::MinAxis:
                vp_h_scale = min_dim;
                vp_v_scale = min_dim;
                vp_h_off = resolution.x > resolution.y ? float(resolution.x - resolution.y) / 2.0f : 0;
                vp_v_off = resolution.y > resolution.x ? float(resolution.y - resolution.x) / 2.0f : 0;
                break;
            case ViewportCoordinateSpaceMode::MaxAxis:
                vp_h_scale = max_dim;
                vp_v_scale = max_dim;
                vp_h_off = resolution.x < resolution.y ? -float(resolution.y - resolution.x) / 2.0f : 0;
                vp_v_off = resolution.y < resolution.x ? -float(resolution.x - resolution.y) / 2.0f : 0;
                break;
            case ViewportCoordinateSpaceMode::HorizontalAxis:
                vp_h_scale = float(resolution.x);
                vp_v_scale = float(resolution.x);
                vp_h_off = 0;
                vp_v_off = (float(resolution.y) - float(resolution.x)) / 2.0f;
                break;
            case ViewportCoordinateSpaceMode::VerticalAxis:
                vp_h_scale = float(resolution.y);
                vp_v_scale = float(resolution.y);
                vp_h_off = (float(resolution.x) - float(resolution.y)) / 2.0f;
                vp_v_off = 0;
                break;
        }

        TransformedViewport transformed{};
        transformed.left = int32_t(viewport.left * vp_h_scale + vp_h_off);
        transformed.right = int32_t(viewport.right * vp_h_scale + vp_h_off);
        transformed.top = int32_t(viewport.top * vp_v_scale + vp_v_off);
        transformed.bottom = int32_t(viewport.bottom * vp_v_scale + vp_v_off);

        return transformed;
    }

    static VkWriteDescriptorSet _create_uniform_ds_write(VkDescriptorSet ds, uint32_t binding,
            const BufferInfo &buffer, VkDescriptorBufferInfo &buf_info) {
        buf_info.buffer = buffer.handle;
        buf_info.offset = 0;
        buf_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet ds_write{};
        ds_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ds_write.dstSet = ds;
        ds_write.dstBinding = binding;
        ds_write.dstArrayElement = 0;
        ds_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_write.descriptorCount = 1;
        ds_write.pBufferInfo = &buf_info;

        return ds_write;
    }

    void draw_scene_to_framebuffer(SceneState &scene_state, ViewportState &viewport_state,
            ValueAndDirtyFlag<Vector2u> resolution) {
        auto &state = scene_state.parent_state;

        auto viewport = viewport_state.viewport->get_viewport();
        auto viewport_px = _transform_viewport_to_pixels(viewport, resolution.value);

        uint32_t fb_width = uint32_t(std::abs(viewport_px.right - viewport_px.left));
        uint32_t fb_height = uint32_t(std::abs(viewport_px.bottom - viewport_px.top));

        if (viewport_state.command_buf.handle == nullptr) {
            viewport_state.command_buf = alloc_command_buffers(state.device, state.command_pool, 1).front();
        }

        if (viewport_state.ubo.handle == VK_NULL_HANDLE) {
            viewport_state.ubo = alloc_buffer(state.device, SHADER_UBO_GLOBAL_LEN, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }

        // framebuffer setup
        if (viewport_state.front_fb == nullptr || resolution.dirty) {
            if (viewport_state.front_fb != nullptr) {
                // delete framebuffers
                destroy_framebuffer(state.device, viewport_state.front_fb);
                destroy_framebuffer(state.device, viewport_state.back_fb);
                destroy_image_and_image_view(state.device, viewport_state.front_fb_tex);
                destroy_image_and_image_view(state.device, viewport_state.back_fb_tex);
            }

            auto format = state.swapchain.image_format;

            viewport_state.front_fb_tex = create_image_and_image_view(state.device, format,
                    { fb_width, fb_height }, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);
            viewport_state.back_fb_tex = create_image_and_image_view(state.device, format,
                    { fb_width, fb_height }, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);
            viewport_state.front_fb = create_framebuffer(state.device, state.render_pass,
                    { viewport_state.front_fb_tex });
            viewport_state.back_fb = create_framebuffer(state.device, state.render_pass,
                    { viewport_state.front_fb_tex });
        }

        begin_oneshot_commands(state.device, viewport_state.command_buf);

        auto vk_cmd_buf = viewport_state.command_buf.handle;

        VkClearValue clear_val{};
        clear_val.color = { {0.0, 0.0, 0.0, 0.0} };

        VkRenderPassBeginInfo rp_info{};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_info.framebuffer = viewport_state.front_fb;
        rp_info.pClearValues = &clear_val;
        rp_info.clearValueCount = 1;
        rp_info.renderPass = state.render_pass;
        rp_info.renderArea.extent = { fb_width, fb_height };
        rp_info.renderArea.offset = { 0, 0 };
        vkCmdBeginRenderPass(vk_cmd_buf, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vk_vp{};
        vk_vp.width = float(fb_width);
        vk_vp.height = float(fb_height);
        vk_vp.x = float(-viewport_px.left);
        vk_vp.y = float(-viewport_px.top);
        vkCmdSetViewport(vk_cmd_buf, 0, 1, &vk_vp);

        VkRect2D scissor{};
        scissor.extent = { fb_width, fb_height };
        scissor.offset = { 0, 0 };
        vkCmdSetScissor(vk_cmd_buf, 0, 1, &scissor);

        affirm_precond(resolution->x <= INT_MAX && resolution->y <= INT_MAX, "Resolution is too big for viewport");

        VkPipeline last_pipeline = nullptr;
        //texture_handle_t last_texture = 0;

        for (auto &bucket : scene_state.render_buckets) {
            affirm_precond(bucket.second->vertex_count <= UINT32_MAX, "Too many vertices in bucket");
            auto vertex_count = uint32_t(bucket.second->vertex_count);

            auto &mat = bucket.second->material_res;
            auto &pipeline_info = state.material_pipelines.find(mat.uid)->second;

            auto &texture_uid = state.material_textures.find(mat.uid)->second;
            auto &texture = state.prepared_textures.find(texture_uid)->second;

            auto &shader_refl = pipeline_info.reflection;

            //auto &texture_uid = mat.get<Material>().get_texture_uid();
            //auto tex_handle = state.prepared_textures.find(texture_uid)->second;

            auto ds_it = viewport_state.material_desc_sets.find(mat.uid);
            std::vector<VkDescriptorSet> desc_sets;
            if (ds_it != viewport_state.material_desc_sets.cend()) {
                desc_sets = ds_it->second;
            } else {
                desc_sets = create_descriptor_sets(state.device, state.desc_pool, shader_refl);
                // bind descriptor set buffers
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = state.global_ubo.handle;
                bufferInfo.offset = 0;
                bufferInfo.range = VK_WHOLE_SIZE;

                std::vector<VkWriteDescriptorSet> ds_writes;

                auto ds = desc_sets[0];

                VkWriteDescriptorSet sampler_ds_write{};
                sampler_ds_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sampler_ds_write.dstSet = ds;
                sampler_ds_write.dstBinding = 0;
                sampler_ds_write.dstArrayElement = 0;
                sampler_ds_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                sampler_ds_write.descriptorCount = 1;
                sampler_ds_write.pImageInfo = nullptr;
                VkDescriptorImageInfo sampler_info{};
                sampler_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                sampler_info.imageView = texture->image.view;
                sampler_info.sampler = texture->sampler;
                sampler_ds_write.pImageInfo = &sampler_info;
                ds_writes.push_back(sampler_ds_write);

                auto global_ubo = state.global_ubo;
                VkDescriptorBufferInfo buf_info_global{};
                VkDescriptorBufferInfo buf_info_viewport{};
                VkDescriptorBufferInfo buf_info_obj{};
                shader_refl.get_ubo_binding_and_then(SHADER_UBO_GLOBAL,
                        [&ds, &ds_writes, &global_ubo, &buf_info_global] (auto binding) {
                            ds_writes.push_back(_create_uniform_ds_write(ds, binding, global_ubo, buf_info_global));
                        });
                auto vp_ubo = viewport_state.ubo;
                shader_refl.get_ubo_binding_and_then(SHADER_UBO_VIEWPORT,
                        [&ds, &ds_writes, &vp_ubo, &buf_info_viewport] (auto binding) {
                            ds_writes.push_back(_create_uniform_ds_write(ds, binding, vp_ubo, buf_info_viewport));
                        });
                auto obj_ubo = bucket.second->ubo_buffer;
                shader_refl.get_ubo_binding_and_then(SHADER_UBO_OBJ,
                        [&ds, &ds_writes, &obj_ubo, &buf_info_obj] (auto binding) {
                            ds_writes.push_back(_create_uniform_ds_write(ds, binding, obj_ubo, buf_info_obj));
                        });

                vkUpdateDescriptorSets(state.device.logical_device, uint32_t(ds_writes.size()), ds_writes.data(),
                        0, nullptr);

                viewport_state.material_desc_sets.insert({ mat.uid, desc_sets });
            }

            auto current_ds = desc_sets[0];

            if (pipeline_info.handle != last_pipeline) {
                vkCmdBindPipeline(vk_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.handle);
                last_pipeline = pipeline_info.handle;
            }

            vkCmdBindDescriptorSets(vk_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_info.layout, 0, 1, &current_ds, 0, nullptr);

            /*if (tex_handle != last_texture) {
                glBindTexture(GL_TEXTURE_2D, tex_handle);
                last_texture = tex_handle;
            }*/

            VkBuffer vertex_buffers[] = { bucket.second->vertex_buffer.handle,
                                          bucket.second->anim_frame_buffer.handle };
            VkDeviceSize offsets[] = { 0, 0 };
            vkCmdBindVertexBuffers(vk_cmd_buf, 0, sizeof(vertex_buffers) / sizeof(VkBuffer),
                    vertex_buffers, offsets);

            //glBindVertexArray(bucket.second->vertex_array);

            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            vkCmdDraw(vk_cmd_buf, vertex_count, 0, 0, 0);
        }

        /*for (auto &postfx : viewport_state.viewport->get_postprocessing_shaders()) {
            LinkedProgram *postfx_program;

            auto &postfx_programs = state.postfx_programs;
            auto it = postfx_programs.find(postfx);
            if (it != postfx_programs.end()) {
                postfx_program = &it->second;
            } else {
                auto linked_program = link_program({FB_SHADER_VERT_PATH, postfx});
                auto inserted = postfx_programs.insert({postfx, linked_program});
                postfx_program = &inserted.first->second;
            }

            std::swap(viewport_state.front_fb, viewport_state.back_fb);
            std::swap(viewport_state.front_frame_tex, viewport_state.back_frame_tex);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.front_fb);

            //glClearColor(0.0, 0.0, 0.0, 0.0);
            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // set viewport

            glBindVertexArray(state.frame_vao);
            glUseProgram(postfx_program->handle);
            set_per_frame_global_uniforms(*postfx_program);
            glBindTexture(GL_TEXTURE_2D, viewport_state.back_frame_tex);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }*/

        vkCmdEndRenderPass(vk_cmd_buf);

        if (vkEndCommandBuffer(vk_cmd_buf) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to record command buffer");
        }
    }

    void draw_framebuffer_to_screen(SceneState &scene_state, ViewportState &viewport_state,
            ValueAndDirtyFlag<Vector2u> resolution) {
        UNUSED(scene_state);
        UNUSED(viewport_state);
        UNUSED(resolution);
        /*auto &state = scene_state.parent_state;

        auto viewport_px = _transform_viewport_to_pixels(viewport_state.viewport->get_viewport(), resolution.value);
        auto viewport_width_px = std::abs(viewport_px.right - viewport_px.left);
        auto viewport_height_px = std::abs(viewport_px.bottom - viewport_px.top);

        auto viewport_y = resolution->y - viewport_px.bottom;
        affirm_precond(resolution->y <= INT_MAX && viewport_y <= INT_MAX, "Viewport Y is too big for glViewport");

        //glViewport(
        //        viewport_px.left,
        //        viewport_y,
        //        viewport_width_px,
        //        viewport_height_px
        //);

        glBindVertexArray(state.frame_vao);
        glUseProgram(state.frame_program.value().handle);
        glBindTexture(GL_TEXTURE_2D, viewport_state.front_frame_tex);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glBindVertexArray(0);*/
    }

    void setup_framebuffer(RendererState &state) {
        UNUSED(state);
        /*auto frame_program = create_pipeline(state, { FB_SHADER_VERT_PATH, FB_SHADER_FRAG_PATH });

        state.frame_program = frame_program;

        if (!frame_program.reflection.get_attr_loc(SHADER_ATTRIB_POSITION).has_value()) {
            Logger::default_logger().fatal("Frame program is missing required position attribute");
        }
        if (!frame_program.reflection.get_attr_loc(SHADER_ATTRIB_TEXCOORD).has_value()) {
            Logger::default_logger().fatal("Frame program is missing required texcoords attribute");
        }

        float frame_quad_vertex_data[] = {
                -1.0, -1.0, 0.0, 0.0,
                -1.0, 1.0, 0.0, 1.0,
                1.0, 1.0, 1.0, 1.0,
                -1.0, -1.0, 0.0, 0.0,
                1.0, 1.0, 1.0, 1.0,
                1.0, -1.0, 1.0, 0.0,
        };

        glCreateVertexArrays(1, &state.frame_vao);

        glCreateBuffers(1, &state.frame_vbo);

        glNamedBufferData(state.frame_vbo, sizeof(frame_quad_vertex_data), frame_quad_vertex_data, GL_STATIC_DRAW);

        glVertexArrayVertexBuffer(state.frame_vao, BINDING_INDEX_VBO, state.frame_vbo, 0,
                4 * uint32_t(sizeof(GLfloat)));

        unsigned int attr_offset = 0;
        set_attrib_pointer(state.frame_vao, state.frame_vbo, BINDING_INDEX_VBO, 4, SHADER_ATTRIB_POSITION_LEN,
                FB_SHADER_ATTRIB_POSITION_LOC, &attr_offset);
        set_attrib_pointer(state.frame_vao, state.frame_vbo, BINDING_INDEX_VBO, 4, SHADER_ATTRIB_TEXCOORD_LEN,
                FB_SHADER_ATTRIB_TEXCOORD_LOC, &attr_offset);*/
    }
}

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

#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/math.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/defines.h"

#include "internal/render_vulkan/renderer/compositing.hpp"
#include "internal/render_vulkan/state/render_bucket.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"
#include "internal/render_vulkan/state/viewport_state.hpp"
#include "internal/render_vulkan/util/command_buffer.hpp"
#include "internal/render_vulkan/util/descriptor_set.hpp"
#include "internal/render_vulkan/util/framebuffer.hpp"
#include "internal/render_vulkan/util/image.hpp"
#include "internal/render_vulkan/util/memory.hpp"

#include "vulkan/vulkan.h"
#include "argus/render/2d/scene_2d.hpp"

#include <map>

#include <climits>

#define BINDING_INDEX_VBO 0

namespace argus {
    struct TransformedViewport {
        int32_t top;
        int32_t bottom;
        int32_t left;
        int32_t right;
    };

    static TransformedViewport _transform_viewport_to_pixels(const Viewport &viewport, const Vector2u &resolution) {
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
            default:
                crash("Unknown ViewportCoordinateSpaceMode ordinal %d", viewport.mode);
        }

        TransformedViewport transformed {};
        transformed.left = int32_t(viewport.left * vp_h_scale + vp_h_off);
        transformed.right = int32_t(viewport.right * vp_h_scale + vp_h_off);
        transformed.top = int32_t(viewport.top * vp_v_scale + vp_v_off);
        transformed.bottom = int32_t(viewport.bottom * vp_v_scale + vp_v_off);

        return transformed;
    }

    [[maybe_unused]] static VkWriteDescriptorSet _create_uniform_ds_write(VkDescriptorSet ds, uint32_t binding,
            const BufferInfo &buffer, VkDescriptorBufferInfo &buf_info) {
        buf_info.buffer = buffer.handle;
        buf_info.offset = 0;
        buf_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet ds_write {};
        ds_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ds_write.dstSet = ds;
        ds_write.dstBinding = binding;
        ds_write.dstArrayElement = 0;
        ds_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_write.descriptorCount = 1;
        ds_write.pBufferInfo = &buf_info;

        return ds_write;
    }

    static void _update_scene_ubo(const RendererState &state, ViewportState &viewport_state) {
        if (!viewport_state.per_frame[state.cur_frame].scene_ubo_dirty) {
            return;
        }

        if (viewport_state.viewport->m_type == SceneType::TwoD) {
            auto &scene = reinterpret_cast<AttachedViewport2D *>(viewport_state.viewport)->get_camera().get_scene();

            auto al_level = scene.peek_ambient_light_level();
            auto al_color = scene.peek_ambient_light_color();

            auto &scene_ubo = viewport_state.per_frame[state.cur_frame].scene_ubo;

            float al_color_arr[3] = { al_color.r, al_color.g, al_color.b };
            write_to_buffer(scene_ubo, al_color_arr, SHADER_UNIFORM_SCENE_AL_COLOR_OFF, sizeof(al_color_arr));

            write_val_to_buffer(scene_ubo, al_level, SHADER_UNIFORM_SCENE_AL_LEVEL_OFF);
        }

        viewport_state.per_frame[state.cur_frame].scene_ubo_dirty = false;
    }

    static void _update_viewport_ubo(const RendererState &state, ViewportState &viewport_state) {
        bool must_update = viewport_state.per_frame[state.cur_frame].view_matrix_dirty;

        auto &viewport_ubo = viewport_state.per_frame[state.cur_frame].viewport_ubo;
        if (viewport_ubo.handle == VK_NULL_HANDLE) {
            viewport_ubo = alloc_buffer(state.device, SHADER_UBO_VIEWPORT_LEN, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    GraphicsMemoryPropCombos::DeviceRw);
            must_update = true;
        }

        if (must_update) {
            write_to_buffer(viewport_ubo, viewport_state.view_matrix.data, SHADER_UNIFORM_VIEWPORT_VM_OFF,
                    sizeof(viewport_state.view_matrix.data));
        }
    }

    static void _create_framebuffers(const RendererState &state, ViewportState &viewport_state, const Vector2u &size) {
        auto format = state.swapchain.image_format;

        for (auto &frame_state : viewport_state.per_frame) {
            frame_state.front_fb.images.resize(2);
            frame_state.back_fb.images.resize(2);

            frame_state.front_fb.images[0] = create_image_and_image_view(state.device, format,
                    { size.x, size.y },
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);
            frame_state.front_fb.images[1] = create_image_and_image_view(state.device, VK_FORMAT_R32_SFLOAT,
                    { size.x, size.y },
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);
            frame_state.back_fb.images[0] = create_image_and_image_view(state.device, format,
                    { size.x, size.y },
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);
            frame_state.back_fb.images[1] = create_image_and_image_view(state.device, VK_FORMAT_R32_SFLOAT,
                    { size.x, size.y },
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);

            frame_state.front_fb.handle = create_framebuffer(state.device, state.fb_render_pass,
                    frame_state.front_fb.images);
            frame_state.back_fb.handle = create_framebuffer(state.device, state.fb_render_pass,
                    frame_state.back_fb.images);

            VkSamplerCreateInfo sampler_info {};
            sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_info.magFilter = VK_FILTER_NEAREST;
            sampler_info.minFilter = VK_FILTER_NEAREST;
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.anisotropyEnable = VK_FALSE;
            sampler_info.maxAnisotropy = 0;
            sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            sampler_info.unnormalizedCoordinates = VK_FALSE;
            sampler_info.compareEnable = VK_FALSE;
            sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sampler_info.mipLodBias = 0.0f;
            sampler_info.minLod = 0.0f;
            sampler_info.maxLod = 0.0f;

            if (vkCreateSampler(state.device.logical_device, &sampler_info, nullptr,
                    &frame_state.front_fb.sampler) != VK_SUCCESS) {
                crash("Failed to create framebuffer sampler");
            }

            frame_state.composite_desc_sets = create_descriptor_sets(state.device, state.desc_pool,
                    state.composite_pipeline.reflection);

            std::vector<VkWriteDescriptorSet> ds_writes;
            for (const auto &ds : frame_state.composite_desc_sets) {
                ds_writes.reserve(frame_state.composite_desc_sets.size());
                VkWriteDescriptorSet sampler_ds_write {};
                sampler_ds_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sampler_ds_write.dstSet = ds;
                sampler_ds_write.dstBinding = 0;
                sampler_ds_write.dstArrayElement = 0;
                sampler_ds_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                sampler_ds_write.descriptorCount = 1;
                sampler_ds_write.pImageInfo = nullptr;
                VkDescriptorImageInfo desc_image_info {};
                desc_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                desc_image_info.imageView = frame_state.front_fb.images[0].view;
                desc_image_info.sampler = frame_state.front_fb.sampler;
                sampler_ds_write.pImageInfo = &desc_image_info;
                ds_writes.push_back(sampler_ds_write);
            }
            vkUpdateDescriptorSets(state.device.logical_device, uint32_t(ds_writes.size()), ds_writes.data(),
                    0, nullptr);
        }
    }

    void draw_scene_to_framebuffer(SceneState &scene_state, ViewportState &viewport_state,
            ValueAndDirtyFlag<Vector2u> resolution) {
        auto &state = scene_state.parent_state;

        auto viewport = viewport_state.viewport->get_viewport();
        auto viewport_px = _transform_viewport_to_pixels(viewport, resolution.value);

        //uint32_t fb_width = uint32_t(std::abs(viewport_px.right - viewport_px.left));
        //uint32_t fb_height = uint32_t(std::abs(viewport_px.bottom - viewport_px.top));
        uint32_t fb_width = state.swapchain.extent.width;
        uint32_t fb_height = state.swapchain.extent.height;

        _update_scene_ubo(state, viewport_state);
        _update_viewport_ubo(state, viewport_state);

        auto &frame_state = viewport_state.per_frame[state.cur_frame];

        // framebuffer setup
        if (frame_state.front_fb.handle == nullptr || resolution.dirty) {
            if (frame_state.front_fb.handle != nullptr) {
                // delete framebuffers
                destroy_framebuffer(state.device, frame_state.front_fb.handle);
                destroy_framebuffer(state.device, frame_state.back_fb.handle);
                for (const auto &image : frame_state.front_fb.images) {
                    destroy_image_and_image_view(state.device, image);
                }
                for (const auto &image : frame_state.back_fb.images) {
                    destroy_image_and_image_view(state.device, image);
                }
                vkDestroySampler(state.device.logical_device, frame_state.front_fb.sampler, nullptr);
                destroy_descriptor_sets(state.device, state.desc_pool, frame_state.composite_desc_sets);
            }

            _create_framebuffers(state, viewport_state, { fb_width, fb_height });
        }

        begin_oneshot_commands(state.device, frame_state.command_buf);

        auto vk_cmd_buf = frame_state.command_buf.handle;

        VkClearValue color_clear_val = {};
        color_clear_val.color = {{ 0.0, 0.0, 0.0, 0.0 }};

        VkClearValue light_opac_clear_val = {};
        light_opac_clear_val.color = {{ 0.0, 0.0, 0.0, 0.0 }};

        VkClearValue clear_vals[] = { color_clear_val, light_opac_clear_val };

        VkRenderPassBeginInfo rp_info {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_info.framebuffer = frame_state.front_fb.handle;
        rp_info.pClearValues = clear_vals;
        rp_info.clearValueCount = sizeof(clear_vals) / sizeof(clear_vals[0]);
        rp_info.renderPass = state.fb_render_pass;
        rp_info.renderArea.extent = { fb_width, fb_height };
        rp_info.renderArea.offset = { 0, 0 };
        vkCmdBeginRenderPass(vk_cmd_buf, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

        affirm_precond(resolution->x <= INT_MAX && resolution->y <= INT_MAX, "Resolution is too big for viewport");

        VkPipeline last_pipeline = nullptr;
        //texture_handle_t last_texture = 0;

        /*VkImageSubresourceRange range{};
        range.layerCount = 1;
        range.baseArrayLayer = 0;
        range.levelCount = 1;
        range.baseMipLevel = 0;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;*/

        /*VkClearAttachment clear_att{};
        clear_att.clearValue = clear_val;
        //clear_att.colorAttachment =
        VkClearRect clear_rect{};
        clear_rect.rect.extent.width =
        vkCmdClearAttachments(vk_cmd_buf, 1, &clear_att, 1, &clear_rect);*/

        for (auto &[_, bucket] : scene_state.render_buckets) {
            affirm_precond(bucket->vertex_count <= UINT32_MAX, "Too many vertices in bucket");
            auto vertex_count = uint32_t(bucket->vertex_count);

            auto &mat = bucket->material_res;
            auto &pipeline_info = state.material_pipelines.find(mat.prototype.uid)->second;

            auto &texture_uid = state.material_textures.find(mat.prototype.uid)->second;
            auto &texture = state.prepared_textures.find(texture_uid)->second;

            auto &shader_refl = pipeline_info.reflection;

            //auto &texture_uid = mat.get<Material>().get_texture_uid();
            //auto tex_handle = state.prepared_textures.find(texture_uid)->second;

            auto ds_it = frame_state.material_desc_sets.find(mat.prototype.uid);
            std::vector<VkDescriptorSet> desc_sets;
            if (ds_it != frame_state.material_desc_sets.cend()) {
                desc_sets = ds_it->second;
            } else {
                desc_sets = create_descriptor_sets(state.device, state.desc_pool, shader_refl);

                std::vector<VkWriteDescriptorSet> ds_writes;

                auto ds = desc_sets[0];

                VkWriteDescriptorSet sampler_ds_write {};
                sampler_ds_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sampler_ds_write.dstSet = ds;
                sampler_ds_write.dstBinding = 0;
                sampler_ds_write.dstArrayElement = 0;
                sampler_ds_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                sampler_ds_write.descriptorCount = 1;
                sampler_ds_write.pImageInfo = nullptr;
                VkDescriptorImageInfo sampler_info {};
                sampler_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                sampler_info.imageView = texture->image.view;
                sampler_info.sampler = texture->sampler;
                sampler_ds_write.pImageInfo = &sampler_info;
                ds_writes.push_back(sampler_ds_write);

                auto global_ubo = state.global_ubo;
                VkDescriptorBufferInfo buf_info_global {};
                VkDescriptorBufferInfo buf_info_scene {};
                VkDescriptorBufferInfo buf_info_viewport {};
                VkDescriptorBufferInfo buf_info_obj {};
                shader_refl.get_ubo_binding_and_then(SHADER_UBO_GLOBAL,
                        [&ds, &ds_writes, &global_ubo, &buf_info_global](auto binding) {
                            ds_writes.push_back(_create_uniform_ds_write(ds, binding, global_ubo, buf_info_global));
                        });
                auto scene_ubo = frame_state.scene_ubo;
                shader_refl.get_ubo_binding_and_then(SHADER_UBO_SCENE,
                        [&ds, &ds_writes, &scene_ubo, &buf_info_scene](auto binding) {
                            ds_writes.push_back(_create_uniform_ds_write(ds, binding, scene_ubo, buf_info_scene));
                        });
                auto vp_ubo = frame_state.viewport_ubo;
                shader_refl.get_ubo_binding_and_then(SHADER_UBO_VIEWPORT,
                        [&ds, &ds_writes, &vp_ubo, &buf_info_viewport](auto binding) {
                            ds_writes.push_back(_create_uniform_ds_write(ds, binding, vp_ubo, buf_info_viewport));
                        });
                auto obj_ubo = bucket->ubo_buffer;
                shader_refl.get_ubo_binding_and_then(SHADER_UBO_OBJ,
                        [&ds, &ds_writes, &obj_ubo, &buf_info_obj](auto binding) {
                            ds_writes.push_back(_create_uniform_ds_write(ds, binding, obj_ubo, buf_info_obj));
                        });

                vkUpdateDescriptorSets(state.device.logical_device, uint32_t(ds_writes.size()), ds_writes.data(),
                        0, nullptr);

                frame_state.material_desc_sets.insert({ mat.prototype.uid, desc_sets });
            }

            auto current_ds = desc_sets[0];

            if (pipeline_info.handle != last_pipeline) {
                vkCmdBindPipeline(vk_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.handle);
                last_pipeline = pipeline_info.handle;

                VkViewport vk_vp {};
                vk_vp.width = float(fb_width);
                vk_vp.height = float(fb_height);
                vk_vp.x = float(-viewport_px.left);
                vk_vp.y = float(-viewport_px.top);
                vkCmdSetViewport(vk_cmd_buf, 0, 1, &vk_vp);

                VkRect2D scissor {};
                scissor.extent = { fb_width, fb_height };
                scissor.offset = { 0, 0 };
                vkCmdSetScissor(vk_cmd_buf, 0, 1, &scissor);
            }

            vkCmdBindDescriptorSets(vk_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_info.layout, 0, 1, &current_ds, 0, nullptr);

            //if (tex_handle != last_texture) {
            //    glBindTexture(GL_TEXTURE_2D, tex_handle);
            //    last_texture = tex_handle;
            //}

            VkBuffer vertex_buffers[] = { bucket->vertex_buffer.handle,
                    bucket->anim_frame_buffer.handle };
            VkDeviceSize offsets[] = { 0, 0 };
            vkCmdBindVertexBuffers(vk_cmd_buf, 0, sizeof(vertex_buffers) / sizeof(VkBuffer),
                    vertex_buffers, offsets);

            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            vkCmdDraw(vk_cmd_buf, vertex_count, 1, 0, 0);
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

            std::swap(viewport_state.fb_primary, viewport_state.fb_secondary);
            std::swap(viewport_state.color_buf_primary, viewport_state.color_buf_secondary);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_primary);

            //glClearColor(0.0, 0.0, 0.0, 0.0);
            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // set viewport

            glBindVertexArray(state.frame_vao);
            glUseProgram(postfx_program->handle);
            set_per_frame_global_uniforms(*postfx_program);
            glBindTexture(GL_TEXTURE_2D, viewport_state.color_buf_secondary);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }*/

        vkCmdEndRenderPass(vk_cmd_buf);

        /*if (vkEndCommandBuffer(vk_cmd_buf) != VK_SUCCESS) {
            crash("Failed to record command buffer");
        }*/
        end_command_buffer(state.device, frame_state.command_buf);
        vkResetFences(state.device.logical_device, 1, &frame_state.composite_fence);
        queue_command_buffer_submit(state, frame_state.command_buf,
                state.device.queues.graphics_family, frame_state.composite_fence,
                { frame_state.rebuild_semaphore }, { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT },
                { frame_state.draw_semaphore }, nullptr);
    }

    void draw_framebuffer_to_swapchain(SceneState &scene_state, ViewportState &viewport_state,
            uint32_t sc_image_index) {
        auto &state = scene_state.parent_state;

        auto resolution = state.swapchain.resolution;

        auto viewport_px = _transform_viewport_to_pixels(viewport_state.viewport->get_viewport(), resolution);

        auto cur_ds = viewport_state.per_frame[state.cur_frame].composite_desc_sets[0];

        std::vector<VkWriteDescriptorSet> ds_writes;

        auto vk_cmd_buf = state.composite_cmd_bufs.find(sc_image_index)->second.first.handle;

        auto fb_width = state.swapchain.resolution.x;
        auto fb_height = state.swapchain.resolution.y;

        VkViewport vk_vp {};
        vk_vp.width = float(fb_width);
        vk_vp.height = float(fb_height);
        vk_vp.x = float(-viewport_px.left);
        vk_vp.y = float(-viewport_px.top);
        vkCmdSetViewport(vk_cmd_buf, 0, 1, &vk_vp);

        VkRect2D scissor {};
        scissor.extent = { fb_width, fb_height };
        scissor.offset = { 0, 0 };
        vkCmdSetScissor(vk_cmd_buf, 0, 1, &scissor);

        vkCmdBindDescriptorSets(vk_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                state.composite_pipeline.layout, 0, 1, &cur_ds, 0, nullptr);

        vkCmdDraw(vk_cmd_buf, 6, 1, 0, 0);
    }

    void setup_framebuffer(RendererState &state) {
        UNUSED(state);
        /*auto frame_program = create_pipeline(state, { FB_SHADER_VERT_PATH, FB_SHADER_FRAG_PATH });

        state.frame_program = frame_program;

        if (!frame_program.reflection.get_attr_loc(SHADER_ATTRIB_POSITION).has_value()) {
            crash("Frame program is missing required position attribute");
        }
        if (!frame_program.reflection.get_attr_loc(SHADER_ATTRIB_TEXCOORD).has_value()) {
            crash("Frame program is missing required texcoords attribute");
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

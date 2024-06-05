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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/threading.hpp"

#include "argus/core/engine_config.hpp"
#include "argus/core/screen_space.hpp"

#include "argus/wm/api_util.hpp"
#include "argus/wm/window.hpp"

#include "argus/render/defines.hpp"
#include "argus/render/common/canvas.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/module_render_vulkan.hpp"
#include "internal/render_vulkan/renderer/bucket_proc.hpp"
#include "internal/render_vulkan/renderer/compositing.hpp"
#include "internal/render_vulkan/renderer/vulkan_renderer.hpp"
#include "internal/render_vulkan/renderer/2d/scene_compiler.hpp"
#include "internal/render_vulkan/setup/swapchain.hpp"
#include "internal/render_vulkan/state/render_bucket.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/util/command_buffer.hpp"
#include "internal/render_vulkan/util/descriptor_set.hpp"
#include "internal/render_vulkan/util/framebuffer.hpp"
#include "internal/render_vulkan/util/image.hpp"
#include "internal/render_vulkan/util/memory.hpp"
#include "internal/render_vulkan/util/pipeline.hpp"
#include "internal/render_vulkan/util/render_pass.hpp"

#include "vulkan/vulkan.h"

#include <algorithm>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace argus {
    using namespace std::chrono_literals;

    // forward declarations
    class Scene2D;

    static float g_frame_quad_vertex_data[] = {
            -1.0, -1.0, 0.0, 0.0,
            -1.0, 1.0, 0.0, 1.0,
            1.0, 1.0, 1.0, 1.0,
            -1.0, -1.0, 0.0, 0.0,
            1.0, 1.0, 1.0, 1.0,
            1.0, -1.0, 1.0, 0.0,
    };

    static Matrix4 _compute_proj_matrix(unsigned int res_hor, unsigned int res_ver) {
        // screen space is [0, 1] on both axes with the origin in the top-left
        auto l = 0;
        auto r = 1;
        auto b = 1;
        auto t = 0;

        float hor_scale = 1;
        float ver_scale = 1;

        auto res_hor_f = float(res_hor);
        auto res_ver_f = float(res_ver);

        switch (get_screen_space_scale_mode()) {
            case ScreenSpaceScaleMode::NormalizeMinDimension:
                if (res_hor > res_ver) {
                    hor_scale = res_hor_f / res_ver_f;
                } else {
                    ver_scale = res_ver_f / res_hor_f;
                }
                break;
            case ScreenSpaceScaleMode::NormalizeMaxDimension:
                if (res_hor > res_ver) {
                    ver_scale = res_ver_f / res_hor_f;
                } else {
                    hor_scale = res_hor_f / res_ver_f;
                }
                break;
            case ScreenSpaceScaleMode::NormalizeVertical:
                hor_scale = res_hor_f / res_ver_f;
                break;
            case ScreenSpaceScaleMode::NormalizeHorizontal:
                ver_scale = res_ver_f / res_hor_f;
                break;
            case ScreenSpaceScaleMode::None:
                break;
        }

        return Matrix4::from_row_major({
                2 / (float(r - l) * hor_scale), 0, 0,
                -float(r + l) /
                        (float(r - l) * hor_scale),
                0, -2 / (float(t - b) * ver_scale), 0, -float(t + b) /
                        -(float(t - b) * ver_scale),
                0, 0, 1, 0,
                0, 0, 0, 1
        });
    }

    static Matrix4 _compute_proj_matrix(const Vector2u &resolution) {
        return _compute_proj_matrix(resolution.x, resolution.y);
    }

    [[maybe_unused]] static Matrix4 _compute_proj_matrix(const Vector2u &&resolution) {
        return _compute_proj_matrix(resolution.x, resolution.y);
    }

    static void _recompute_2d_viewport_view_matrix(const Viewport &viewport, const Transform2D &transform,
            const Vector2u &resolution, Matrix4 &dest) {
        UNUSED(transform);

        auto center_x = (viewport.left + viewport.right) / 2.0f;
        auto center_y = (viewport.top + viewport.bottom) / 2.0f;

        auto cur_translation = transform.get_translation();

        Matrix4 anchor_mat_1 = Matrix4::from_row_major({
                1, 0, 0, -center_x + cur_translation.x,
                0, 1, 0, -center_y + cur_translation.y,
                0, 0, 1, 0,
                0, 0, 0, 1,
        });
        Matrix4 anchor_mat_2 = Matrix4::from_row_major({
                1, 0, 0, center_x - cur_translation.x,
                0, 1, 0, center_y - cur_translation.y,
                0, 0, 1, 0,
                0, 0, 0, 1,
        });
        auto view_mat = transform.get_translation_matrix()
                * anchor_mat_2
                * transform.get_rotation_matrix()
                * transform.get_scale_matrix()
                * anchor_mat_1;

        dest = _compute_proj_matrix(resolution) * view_mat;
    }

    static std::set<Scene *> _get_associated_scenes_for_canvas(Canvas &canvas) {
        std::set<Scene *> scenes;
        for (auto viewport : canvas.get_viewports_2d()) {
            scenes.insert(&viewport.get().get_camera().get_scene());
        }
        return scenes;
    }

    static void _try_free_buffer(BufferInfo &buffer) {
        if (buffer.handle != nullptr) {
            free_buffer(buffer);
            buffer.handle = nullptr;
        }
    }

    static Viewport2DState &_create_viewport_2d_state(RendererState &state, AttachedViewport2D &viewport) {
        auto [inserted, success] = state.viewport_states_2d.try_emplace(&viewport, state, &viewport);
        if (!success) {
            Logger::default_logger().fatal("Failed to create new viewport state");
        }

        auto &viewport_state = inserted->second;

        VkSemaphoreCreateInfo sem_info {};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (auto &frame_state : viewport_state.per_frame) {
            if (vkCreateSemaphore(state.device.logical_device, &sem_info, nullptr,
                    &frame_state.rebuild_semaphore) != VK_SUCCESS) {
                Logger::default_logger().fatal("Failed to create semaphores for viewport");
            }

            if (vkCreateSemaphore(state.device.logical_device, &sem_info, nullptr,
                    &frame_state.draw_semaphore) != VK_SUCCESS) {
                Logger::default_logger().fatal("Failed to create semaphores for viewport");
            }

            VkFenceCreateInfo fence_info {};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = 0;
            if (vkCreateFence(state.device.logical_device, &fence_info, nullptr,
                    &frame_state.composite_fence) != VK_SUCCESS) {
                Logger::default_logger().fatal("Failed to create fences for viewport");
            }

            frame_state.command_buf = alloc_command_buffers(state.device, state.graphics_command_pool, 1).front();

            frame_state.scene_ubo = alloc_buffer(state.device, SHADER_UBO_SCENE_LEN,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GraphicsMemoryPropCombos::HostRw);
        }

        return inserted->second;
    }

    static Scene2DState &_create_scene_state(RendererState &state, Scene2D &scene) {
        auto [inserted, success] = state.scene_states_2d.try_emplace(&scene, state, scene);
        if (!success) {
            Logger::default_logger().fatal("Failed to create new scene state");
        }

        inserted->second.scene_ubo_staging = alloc_buffer(state.device, SHADER_UBO_SCENE_LEN,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GraphicsMemoryPropCombos::HostRw);

        return inserted->second;
    }

    static void _destroy_viewport(const RendererState &state, ViewportState &viewport_state) {
        for (auto &frame_state : viewport_state.per_frame) {
            vkDestroyFence(state.device.logical_device, frame_state.composite_fence, nullptr);
            vkDestroySampler(state.device.logical_device, frame_state.front_fb.sampler, nullptr);
            destroy_framebuffer(state.device, frame_state.front_fb.handle);
            destroy_framebuffer(state.device, frame_state.back_fb.handle);
            for (const auto &image : frame_state.front_fb.images) {
                destroy_image_and_image_view(state.device, image);
            }
            for (const auto &image : frame_state.front_fb.images) {
                destroy_image_and_image_view(state.device, image);
            }
            free_buffer(frame_state.viewport_ubo);
            destroy_descriptor_sets(state.device, state.desc_pool, frame_state.composite_desc_sets);
            for (const auto &[_, ds] : frame_state.material_desc_sets) {
                destroy_descriptor_sets(state.device, state.desc_pool, ds);
            }

            free_command_buffer(state.device, frame_state.command_buf);
        }
    }

    static void _destroy_scene(const RendererState &state, SceneState &scene_state) {
        UNUSED(state);

        for (const auto &[_, bucket] : scene_state.render_buckets) {
            for (auto *pro : bucket->objects) {
                deinit_object_2d(state, *pro);
            }

            _try_free_buffer(bucket->vertex_buffer);
            _try_free_buffer(bucket->anim_frame_buffer);
            _try_free_buffer(bucket->staging_vertex_buffer);
            _try_free_buffer(bucket->staging_anim_frame_buffer);
            _try_free_buffer(bucket->ubo_buffer);

            bucket->destroy();
        }

        free_buffer(scene_state.scene_ubo_staging);
    }

    static void _add_remove_state_objects(const Window &window, RendererState &state) {
        auto &canvas = window.get_canvas();

        for (auto &viewport : canvas.get_viewports_2d()) {
            auto vp_it = state.viewport_states_2d.find(&viewport.get());
            ViewportState *vp_state;
            if (vp_it != state.viewport_states_2d.end()) {
                vp_state = &vp_it->second;
            } else {
                state.dirty_viewports = true;
                vp_state = &_create_viewport_2d_state(state, viewport.get());
            }

            vp_state->visited = true;

            auto &scene = viewport.get().get_camera().get_scene();
            auto scene_it = state.scene_states_2d.find(&scene);
            SceneState *scene_state;
            if (scene_it != state.scene_states_2d.end()) {
                scene_state = &scene_it->second;
            } else {
                scene_state = &_create_scene_state(state, scene);
            }

            scene_state->visited = true;
        }

        for (auto it = state.scene_states_2d.begin(); it != state.scene_states_2d.end();) {
            if (!it->second.visited) {
                _destroy_scene(state, it->second);
                it = state.scene_states_2d.erase(it);
            } else {
                it->second.visited = false;
                it++;
            }
        }

        for (auto it = state.viewport_states_2d.begin(); it != state.viewport_states_2d.end();) {
            if (!it->second.visited) {
                _destroy_viewport(state, it->second);
                it = state.viewport_states_2d.erase(it);

                state.dirty_viewports = true;
            } else {
                it->second.visited = false;
                it++;
            }
        }
    }

    static void _update_view_matrix(const Window &window, RendererState &state, const Vector2u &resolution) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            auto &viewport_state
                    = reinterpret_cast<Viewport2DState &>(state.get_viewport_state(viewport));
            auto camera_transform = viewport.get().get_camera().peek_transform();
            _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform.inverse(),
                    resolution,
                    viewport_state.view_matrix);
            for (auto &frame_state : viewport_state.per_frame) {
                frame_state.view_matrix_dirty = true;
            }
        }
    }

    static void _recompute_viewports(const Window &window, RendererState &state) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            Viewport2DState &viewport_state
                    = reinterpret_cast<Viewport2DState &>(state.get_viewport_state(viewport));
            auto camera_transform = viewport.get().get_camera().get_transform();

            if (camera_transform.dirty) {
                _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform->inverse(),
                        window.peek_resolution(), viewport_state.view_matrix);
            }
        }
    }

    static void _compile_scenes(const Window &window, RendererState &state) {
        auto &canvas = window.get_canvas();

        for (auto *scene : _get_associated_scenes_for_canvas(canvas)) {
            SceneState &scene_state = state.get_scene_state(*scene);

            compile_scene_2d(reinterpret_cast<Scene2D &>(*scene), reinterpret_cast<Scene2DState &>(scene_state));
        }
    }

    static void _check_scene_ubo_dirty(SceneState &scene_state) {
        if (scene_state.scene.type == SceneType::TwoD) {
            auto &scene = reinterpret_cast<Scene2D &>(scene_state.scene);

            auto al_level = scene.get_ambient_light_level();
            auto al_color = scene.get_ambient_light_color();

            bool must_update = al_level.dirty || al_color.dirty;

            if (must_update) {
                /*auto &staging_ubo = scene_state.scene_ubo_staging;

                if (al_color.dirty) {
                    float al_color_arr[3] = { al_color->r, al_color->g, al_color->b };
                    write_to_buffer(staging_ubo, al_color_arr, SHADER_UNIFORM_SCENE_AL_COLOR_OFF, sizeof(al_color_arr));
                }

                if (al_level.dirty) {
                    write_val_to_buffer(staging_ubo, al_level.value, SHADER_UNIFORM_SCENE_AL_LEVEL_OFF);
                }*/

                for (auto &viewport_state : scene_state.parent_state.viewport_states_2d) {
                    for (auto &per_frame : viewport_state.second.per_frame) {
                        per_frame.scene_ubo_dirty = true;
                    }
                }
            }
        }
    }

    static void _record_scene_rebuild(const Window &window, RendererState &state) {
        auto &canvas = window.get_canvas();

        begin_oneshot_commands(state.device, state.copy_cmd_buf[state.cur_frame]);

        for (auto *scene : _get_associated_scenes_for_canvas(canvas)) {
            SceneState &scene_state = state.get_scene_state(*scene);

            _check_scene_ubo_dirty(scene_state);

            fill_buckets(scene_state);

            for (const auto &[_, bucket] : scene_state.render_buckets) {
                auto &mat = bucket->material_res;
                UNUSED(mat);

                get_or_load_texture(state, mat);
            }
        }

        end_command_buffer(state.device, state.copy_cmd_buf[state.cur_frame]);
    }

    static void _submit_scene_rebuild(RendererState &state) {
        std::vector<VkSemaphore> rebuild_sems;
        rebuild_sems.resize(state.viewport_states_2d.size());
        std::transform(state.viewport_states_2d.begin(), state.viewport_states_2d.end(), rebuild_sems.begin(),
                [](const auto &kv) {
                    return kv.second.per_frame[kv.second.parent_state.cur_frame]
                            .rebuild_semaphore;
                });
        queue_command_buffer_submit(state, state.copy_cmd_buf[state.cur_frame], state.device.queues.graphics_family,
                VK_NULL_HANDLE, {}, {}, rebuild_sems, nullptr);

        /*for (auto &buf : state.texture_bufs_to_free) {
            free_buffer(buf);
        }
        state.texture_bufs_to_free.clear();*/
    }

    static uint32_t _get_next_image(RendererState &state) {
        auto &device = state.device.logical_device;

        state.in_flight_sem[state.cur_frame].wait();
        vkWaitForFences(device, 1, &state.swapchain.in_flight_fence[state.cur_frame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &state.swapchain.in_flight_fence[state.cur_frame]);

        uint32_t image_index = 0;

        std::lock_guard<std::mutex> submit_lock(state.submit_mutex);

        vkAcquireNextImageKHR(device, state.swapchain.handle, UINT64_MAX,
                state.swapchain.image_avail_sem[state.cur_frame], VK_NULL_HANDLE, &image_index);

        return image_index;
    }

    static void _composite_framebuffers(RendererState &state,
            const std::vector<std::reference_wrapper<AttachedViewport2D>> &viewports, uint32_t sc_image_index) {
        CommandBufferInfo *cmd_buf;

        auto cb_it = state.composite_cmd_bufs.find(sc_image_index);
        if (cb_it != state.composite_cmd_bufs.cend()) {
            if (!cb_it->second.second) {
                return;
            }

            cb_it->second.second = false;

            cmd_buf = &cb_it->second.first;
        } else {
            auto new_cmd_buf = alloc_command_buffers(state.device, state.graphics_command_pool, 1).front();
            cmd_buf = &state.composite_cmd_bufs.insert({ sc_image_index, std::make_pair(new_cmd_buf, false) })
                    .first->second.first;
        }

        vkResetCommandBuffer(cmd_buf->handle, 0);

        VkCommandBufferBeginInfo cmd_begin_info {};
        cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_begin_info.flags = 0;
        vkBeginCommandBuffer(cmd_buf->handle, &cmd_begin_info);

        auto vk_cmd_buf = cmd_buf->handle;

        auto fb_width = state.swapchain.extent.width;
        auto fb_height = state.swapchain.extent.height;

        VkClearValue clear_val {};
        clear_val.color = {{ 0.0, 0.0, 0.0, 0.0 }};

        VkRenderPassBeginInfo rp_info {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_info.framebuffer = state.swapchain.framebuffers[sc_image_index];
        rp_info.pClearValues = &clear_val;
        rp_info.clearValueCount = 1;
        rp_info.renderPass = state.swapchain.composite_render_pass;
        rp_info.renderArea.extent = { fb_width, fb_height };
        rp_info.renderArea.offset = { 0, 0 };
        vkCmdBeginRenderPass(vk_cmd_buf, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(vk_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, state.composite_pipeline.handle);

        VkDeviceSize offsets[] = { 0, 0 };
        vkCmdBindVertexBuffers(vk_cmd_buf, 0, 1, &state.composite_vbo.handle, offsets);

        for (auto &viewport : viewports) {
            auto &viewport_state = state.get_viewport_state(viewport);
            auto &scene = viewport.get().get_camera().get_scene();
            auto &scene_state = state.get_scene_state(scene);

            draw_framebuffer_to_swapchain(scene_state, viewport_state, sc_image_index);
        }

        vkCmdEndRenderPass(vk_cmd_buf);

        vkEndCommandBuffer(cmd_buf->handle);
    }

    static void _submit_composite(RendererState &state, uint32_t sc_image_index) {
        std::vector<VkSemaphore> wait_sems;
        std::vector<VkPipelineStageFlags> wait_stages;
        wait_sems.reserve(state.viewport_states_2d.size());
        wait_sems.push_back(state.swapchain.image_avail_sem[state.cur_frame]);
        wait_stages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        for (const auto &[_, viewport_state] : state.viewport_states_2d) {
            wait_sems.push_back(viewport_state.per_frame[state.cur_frame].draw_semaphore);
            wait_stages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }

        std::vector<VkSemaphore> signal_sems = { state.swapchain.render_done_sem[state.cur_frame] };

        queue_command_buffer_submit(state, state.composite_cmd_bufs.find(sc_image_index)->second.first,
                state.device.queues.graphics_family, state.swapchain.in_flight_fence[state.cur_frame],
                wait_sems, wait_stages, { state.swapchain.render_done_sem[state.cur_frame] },
                &state.in_flight_sem[state.cur_frame]);
    }

    static void _present_image(RendererState &state, uint32_t image_index) {
        state.submit_bufs.push_back(CommandBufferSubmitParams { true, image_index, state.cur_frame, nullptr,
                VK_NULL_HANDLE, VK_NULL_HANDLE, {}, {}, {}, &state.present_sem[state.cur_frame] });
        state.queued_submit_sem.notify();
    }

    static void *_submit_queues_loop(void *state_ptr) {
        auto &state = *reinterpret_cast<RendererState *>(state_ptr);

        while (true) {
            if (state.submit_halt) {
                state.submit_halt_acked.notify();
                return nullptr;
            }

            state.queued_submit_sem.wait();

            std::lock_guard<std::mutex> submit_lock(state.submit_mutex);
            std::lock_guard<std::mutex> queue_lock(state.device.queue_mutexes->graphics_family);

            while (!state.submit_bufs.empty()) {
                const auto &buf = state.submit_bufs.front();

                if (buf.is_present) {
                    VkPresentInfoKHR present_info {};
                    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                    present_info.waitSemaphoreCount = 1;
                    present_info.pWaitSemaphores = &state.swapchain.render_done_sem[buf.cur_frame];
                    present_info.swapchainCount = 1;
                    present_info.pSwapchains = &state.swapchain.handle;
                    present_info.pImageIndices = &buf.present_image_index;
                    present_info.pResults = nullptr;

                    vkQueuePresentKHR(state.device.queues.graphics_family, &present_info);

                    if (buf.submit_sem != nullptr) {
                        buf.submit_sem->notify();
                    }
                } else {
                    submit_command_buffer(state.device, *buf.buffer, buf.queue, buf.fence,
                            buf.wait_sems, buf.wait_stages, buf.signal_sems);

                    if (buf.submit_sem != nullptr) {
                        buf.submit_sem->notify();
                    }
                }

                state.submit_bufs.pop_front();
            }
        }
    }

    VulkanRenderer::VulkanRenderer(Window &window):
        m_window(window),
        m_is_initted(false) {
        m_state.device = g_vk_device;
    }

    VulkanRenderer::~VulkanRenderer(void) {
        if (!m_is_initted) {
            return;
        }

        m_state.submit_halt = true;
        m_state.queued_submit_sem.notify();
        m_state.submit_halt_acked.wait();
        m_state.submit_thread.join();

        {
            for (auto &sem : m_state.present_sem) {
                sem.wait();
            }
            std::lock_guard<std::mutex> queue_lock(m_state.device.queue_mutexes->graphics_family);
            vkQueueWaitIdle(m_state.device.queues.graphics_family);
        }

        for (auto &[_, viewport_state] : m_state.viewport_states_2d) {
            _destroy_viewport(m_state, viewport_state);
        }

        for (auto &[_, scene_state] : m_state.scene_states_2d) {
            _destroy_scene(m_state, scene_state);
        }

        for (auto &cb : m_state.copy_cmd_buf) {
            if (cb.handle != VK_NULL_HANDLE) {
                free_command_buffer(m_state.device, cb);
            }
        }

        for (const auto &[_, comp_cmd_buf] : m_state.composite_cmd_bufs) {
            if (comp_cmd_buf.first.handle != VK_NULL_HANDLE) {
                free_command_buffer(m_state.device, comp_cmd_buf.first);
            }
        }
        m_state.composite_cmd_bufs.clear();

        if (m_state.composite_vbo.handle != VK_NULL_HANDLE) {
            free_buffer(m_state.composite_vbo);
        }

        if (m_state.global_ubo.handle != VK_NULL_HANDLE) {
            free_buffer(m_state.global_ubo);
        }

        if (m_state.desc_pool != VK_NULL_HANDLE) {
            destroy_descriptor_pool(m_state.device, m_state.desc_pool);
        }

        if (m_state.composite_pipeline.handle != VK_NULL_HANDLE) {
            destroy_pipeline(m_state.device, m_state.composite_pipeline);
        }

        for (const auto &[_, pipeline] : m_state.material_pipelines) {
            destroy_pipeline(m_state.device, pipeline);
        }

        if (m_state.fb_render_pass != VK_NULL_HANDLE) {
            destroy_render_pass(m_state.device, m_state.fb_render_pass);
        }

        if (m_state.graphics_command_pool != VK_NULL_HANDLE) {
            destroy_command_pool(m_state.device, m_state.graphics_command_pool);
        }

        for (const auto &[_, texture] : m_state.prepared_textures) {
            destroy_texture(m_state.device, texture.value);
        }

        destroy_swapchain(m_state, m_state.swapchain);

        vkDestroySurfaceKHR(g_vk_instance, m_state.surface, nullptr);
    }

    void VulkanRenderer::init(void) {
        if (!vk_create_surface(m_window, reinterpret_cast<VkDevice *>(g_vk_instance),
                reinterpret_cast<void **>(&m_state.surface))) {
            Logger::default_logger().fatal("Failed to create Vulkan surface");
        }
        Logger::default_logger().debug("Created surface for new window");

        m_state.graphics_command_pool = create_command_pool(this->m_state.device,
                this->m_state.device.queue_indices.graphics_family);
        Logger::default_logger().debug("Created command pools for new window");

        m_state.desc_pool = create_descriptor_pool(this->m_state.device);
        Logger::default_logger().debug("Created descriptor pool for new window");

        auto copy_cmd_bufs = alloc_command_buffers(m_state.device, m_state.graphics_command_pool, MAX_FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_state.copy_cmd_buf[i] = copy_cmd_bufs[i];
        }
        Logger::default_logger().debug("Created command buffers for new window");

        /*VkSemaphoreCreateInfo sem_info{};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(state.device.logical_device, &sem_info, nullptr, &state.rebuild_semaphore)
                != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create semaphores for new window");
        }
        Logger::default_logger().debug("Created semaphores for new window");*/

        m_state.global_ubo = alloc_buffer(this->m_state.device, SHADER_UBO_GLOBAL_LEN,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GraphicsMemoryPropCombos::DeviceRw);

        m_state.submit_thread = std::thread(_submit_queues_loop, &m_state);

        m_state.swapchain = create_swapchain(this->m_state, m_state.surface, this->m_window.peek_resolution());
        Logger::default_logger().debug("Created swapchain for new window");

        m_state.composite_pipeline = create_pipeline(this->m_state, { FB_SHADER_VERT_PATH, FB_SHADER_FRAG_PATH },
                this->m_state.swapchain.composite_render_pass);
        Logger::default_logger().debug("Created composite pipeline");

        m_state.composite_vbo = alloc_buffer(m_state.device, sizeof(g_frame_quad_vertex_data),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                GraphicsMemoryPropCombos::DeviceRw);
        memcpy(m_state.composite_vbo.mapped, g_frame_quad_vertex_data, sizeof(g_frame_quad_vertex_data));
        Logger::default_logger().debug("Created composite VBO");

        m_state.fb_render_pass = create_render_pass(this->m_state.device, this->m_state.swapchain.image_format,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);
        Logger::default_logger().debug("Created framebuffer render pass for new window");

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_state.present_sem[i].notify();
            m_state.in_flight_sem[i].notify();
        }

        m_is_initted = true;
    }

    void VulkanRenderer::render(TimeDelta delta) {
        UNUSED(delta);

        static auto last_print = std::chrono::high_resolution_clock::now();
        static int64_t time_samples = 0;

        static std::chrono::nanoseconds compile_time;
        static std::chrono::nanoseconds rebuild_time;
        static std::chrono::nanoseconds draw_time;
        static std::chrono::nanoseconds composite_time;

        std::chrono::high_resolution_clock::time_point timer_start;
        std::chrono::high_resolution_clock::time_point timer_end;

        if (std::chrono::high_resolution_clock::now() - last_print >= 10s && time_samples > 0) {
            Logger::default_logger().debug("Compile + rebuild + draw + composite took %ld + %ld + %ld + %ld ns\n",
                    compile_time.count() / time_samples,
                    rebuild_time.count() / time_samples,
                    draw_time.count() / time_samples,
                    composite_time.count() / time_samples);

            compile_time = rebuild_time = draw_time = composite_time = 0ns;
            time_samples = 0;
            last_print = std::chrono::high_resolution_clock::now();
        }

        auto vsync = m_window.is_vsync_enabled();
        if (vsync.dirty) {
            //glfwSwapInterval(vsync ? 1 : 0);
        }

        _add_remove_state_objects(m_window, m_state);

        if (!m_state.are_viewports_initialized) {
            _update_view_matrix(m_window, m_state, m_window.get_resolution().value);
            m_state.are_viewports_initialized = true;
        }

        timer_start = std::chrono::high_resolution_clock::now();
        _recompute_viewports(m_window, m_state);
        _compile_scenes(m_window, m_state);
        timer_end = std::chrono::high_resolution_clock::now();
        compile_time += (timer_end - timer_start);

        {
            //state.present_sem[state.cur_frame].wait();
        }

        auto sc_image_index = _get_next_image(m_state);

        timer_start = std::chrono::high_resolution_clock::now();
        _record_scene_rebuild(m_window, m_state);
        _submit_scene_rebuild(m_state);
        timer_end = std::chrono::high_resolution_clock::now();
        rebuild_time += (timer_end - timer_start);

        auto &canvas = m_window.get_canvas();

        auto resolution = m_window.get_resolution();

        auto viewports = canvas.get_viewports_2d();
        std::sort(viewports.begin(), viewports.end(),
                [](auto a, auto b) { return a.get().get_z_index() < b.get().get_z_index(); });

        timer_start = std::chrono::high_resolution_clock::now();
        for (auto viewport : viewports) {
            auto &viewport_state = m_state.get_viewport_state(viewport);
            auto &scene = viewport.get().get_camera().get_scene();
            auto &scene_state = m_state.get_scene_state(scene);

            draw_scene_to_framebuffer(scene_state, viewport_state, resolution);
        }

        timer_end = std::chrono::high_resolution_clock::now();
        draw_time += (timer_end - timer_start);

        // set up state for drawing framebuffers to screen

        //glClearColor(0.0, 0.0, 0.0, 1.0);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        timer_start = std::chrono::high_resolution_clock::now();

        if (m_state.dirty_viewports || resolution.dirty) {
            for (auto &[_, cmd_buf] : m_state.composite_cmd_bufs) {
                cmd_buf.second = true;
            }
            m_state.dirty_viewports = false;
        }

        _composite_framebuffers(m_state, viewports, sc_image_index);

        _submit_composite(m_state, sc_image_index);
        timer_end = std::chrono::high_resolution_clock::now();
        composite_time += (timer_end - timer_start);

        _present_image(m_state, sc_image_index);

        if ((++m_state.cur_frame) >= MAX_FRAMES_IN_FLIGHT) {
            m_state.cur_frame = 0;
        }

        time_samples++;
    }

    void VulkanRenderer::notify_window_resize(const Vector2u &resolution) {
        recreate_swapchain(m_state, resolution, m_state.swapchain);
        _update_view_matrix(this->m_window, this->m_state, resolution);
    }
}

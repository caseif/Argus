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

#include "argus/core/engine_config.hpp"
#include "argus/core/screen_space.hpp"

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
#include "internal/render_vulkan/util/pipeline.hpp"
#include "internal/render_vulkan/util/render_pass.hpp"

#pragma GCC diagnostic push

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdocumentation"
#endif
#include "GLFW/glfw3.h"
#pragma GCC diagnostic pop
#include "vulkan/vulkan.h"

namespace argus {
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

    static Matrix4 _compute_view_matrix(unsigned int res_hor, unsigned int res_ver) {
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
                2 / (float(r - l) * hor_scale), 0,                              0,
                                                                                    -float(r + l) /
                                                                                    (float(r - l) * hor_scale),
                0,                              -2 / (float(t - b) * ver_scale), 0, -float(t + b) /
                                                                                    -(float(t - b) * ver_scale),
                0,                              0,                              1, 0,
                0,                              0,                              0, 1
        });
    }

    static Matrix4 _compute_view_matrix(const Vector2u &resolution) {
        return _compute_view_matrix(resolution.x, resolution.y);
    }

    [[maybe_unused]] static Matrix4 _compute_view_matrix(const Vector2u &&resolution) {
        return _compute_view_matrix(resolution.x, resolution.y);
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
        dest = Matrix4::identity();
        multiply_matrices(dest, anchor_mat_1);
        multiply_matrices(dest, transform.get_scale_matrix());
        multiply_matrices(dest, transform.get_rotation_matrix());
        multiply_matrices(dest, anchor_mat_2);
        multiply_matrices(dest, transform.get_translation_matrix());
        multiply_matrices(dest, _compute_view_matrix(resolution));
    }

    static std::set<Scene *> _get_associated_scenes_for_canvas(Canvas &canvas) {
        std::set<Scene *> scenes;
        for (auto viewport : canvas.get_viewports_2d()) {
            scenes.insert(&viewport.get().get_camera().get_scene());
        }
        return scenes;
    }

    static void _destroy_viewport(const RendererState &state, const ViewportState &viewport_state) {
        vkDestroySampler(state.device.logical_device, viewport_state.front_fb_sampler, nullptr);
        destroy_framebuffer(state.device, viewport_state.front_fb);
        destroy_framebuffer(state.device, viewport_state.back_fb);
        destroy_image(state.device, viewport_state.front_fb_image.handle);
        destroy_image(state.device, viewport_state.back_fb_image.handle);
        free_buffer(state.device, viewport_state.ubo);
        destroy_descriptor_sets(state.device, state.desc_pool, viewport_state.composite_desc_sets);
        for (const auto &ds : viewport_state.material_desc_sets) {
            destroy_descriptor_sets(state.device, state.desc_pool, ds.second);
        }
        free_command_buffer(state.device, viewport_state.command_buf);
    }

    static void _update_view_matrix(const Window &window, RendererState &state, const Vector2u &resolution) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            auto &viewport_state
                    = reinterpret_cast<Viewport2DState &>(state.get_viewport_state(viewport, true));
            auto camera_transform = viewport.get().get_camera().peek_transform();
            _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform.inverse(),
                    resolution,
                    viewport_state.view_matrix);
            viewport_state.view_matrix_dirty = true;
        }
    }

    static void _rebuild_scene(const Window &window, RendererState &state) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            Viewport2DState &viewport_state
                    = reinterpret_cast<Viewport2DState &>(state.get_viewport_state(viewport, true));
            auto camera_transform = viewport.get().get_camera().get_transform();

            if (camera_transform.dirty) {
                _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform->inverse(),
                        window.peek_resolution(), viewport_state.view_matrix);
            }
        }

        begin_oneshot_commands(state.device, state.copy_cmd_buf);

        for (auto *scene : _get_associated_scenes_for_canvas(canvas)) {
            SceneState &scene_state = state.get_scene_state(*scene, true);

            compile_scene_2d(reinterpret_cast<Scene2D &>(*scene), reinterpret_cast<Scene2DState &>(scene_state));

            fill_buckets(scene_state);

            for (const auto &bucket_it : scene_state.render_buckets) {
                auto &mat = bucket_it.second->material_res;
                UNUSED(mat);

                get_or_load_texture(state, mat);
            }
        }

        end_oneshot_commands(state.device, state.copy_cmd_buf);

        for (const auto &buf : state.texture_bufs_to_free) {
            free_buffer(state.device, buf);
        }
        state.texture_bufs_to_free.clear();
    }

    static uint32_t _get_next_image(const RendererState &state) {
        auto &device = state.device.logical_device;

        vkWaitForFences(device, 1, &state.swapchain.in_flight_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &state.swapchain.in_flight_fence);

        uint32_t image_index = 0;

        vkAcquireNextImageKHR(device, state.swapchain.handle, UINT64_MAX, state.swapchain.image_avail_sem,
                VK_NULL_HANDLE, &image_index);

        return image_index;
    }

    static void _submit_queues(const RendererState &state) {
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_sems[] = { state.swapchain.image_avail_sem };
        VkSemaphore signal_sems[] = { state.swapchain.render_done_sem };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submit_info.waitSemaphoreCount = sizeof(wait_sems) / sizeof(VkSemaphore);
        submit_info.pWaitSemaphores = wait_sems;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &state.draw_cmd_buf.handle;
        submit_info.signalSemaphoreCount = sizeof(signal_sems) / sizeof(VkSemaphore);
        submit_info.pSignalSemaphores = signal_sems;

        if (vkQueueSubmit(state.device.queues.graphics_family, 1, &submit_info, state.swapchain.in_flight_fence)
            != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to submit draw command buffer");
        }
    }

    static void _present_image(const RendererState &state, uint32_t image_index) {
        VkSwapchainKHR swapchains[] = { state.swapchain.handle };

        VkSemaphore signal_sems[] = { state.swapchain.render_done_sem };

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_sems;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &image_index;
        present_info.pResults = nullptr;

        vkQueuePresentKHR(state.device.queues.present_family, &present_info);
    }

    VulkanRenderer::VulkanRenderer(Window &window):
        resource_event_handler(),
        window(window),
        is_initted(false) {
        UNUSED(this->resource_event_handler);

        state.device = g_vk_device;

        auto surface_err = glfwCreateWindowSurface(g_vk_instance,
                get_window_handle<GLFWwindow>(window), nullptr, &state.surface);
        if (surface_err) {
            Logger::default_logger().fatal("glfwCreateWindowSurface returned error code %d", surface_err);
        }
        Logger::default_logger().debug("Created surface for new window");

        state.command_pool = create_command_pool(this->state.device);
        Logger::default_logger().debug("Created command pool for new window");

        state.desc_pool = create_descriptor_pool(this->state.device);
        Logger::default_logger().debug("Created descriptor pool for new window");

        state.copy_cmd_buf = alloc_command_buffers(state.device, state.command_pool, 1).front();
        state.draw_cmd_buf = alloc_command_buffers(state.device, state.command_pool, 1).front();
        Logger::default_logger().debug("Created command buffers for new window");

        state.global_ubo = alloc_buffer(this->state.device, SHADER_UBO_GLOBAL_LEN, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    VulkanRenderer::~VulkanRenderer(void) {
        vkQueueWaitIdle(state.device.queues.graphics_family);

        for (const auto &viewport_state : state.viewport_states_2d) {
            _destroy_viewport(state, viewport_state.second);
        }

        if (state.copy_cmd_buf.handle != VK_NULL_HANDLE) {
            free_command_buffer(state.device, state.copy_cmd_buf);
        }

        if (state.draw_cmd_buf.handle != VK_NULL_HANDLE) {
            free_command_buffer(state.device, state.draw_cmd_buf);
        }

        if (state.composite_vbo.handle != VK_NULL_HANDLE) {
            free_buffer(state.device, state.composite_vbo);
        }

        if (state.global_ubo.handle != VK_NULL_HANDLE) {
            free_buffer(state.device, state.global_ubo);
        }

        if (state.desc_pool != VK_NULL_HANDLE) {
            destroy_descriptor_pool(state.device, state.desc_pool);
        }

        if (state.composite_pipeline.handle != VK_NULL_HANDLE) {
            destroy_pipeline(state.device, state.composite_pipeline);
        }

        for (const auto &pipeline : state.material_pipelines) {
            destroy_pipeline(state.device, pipeline.second);
        }

        if (state.fb_render_pass != VK_NULL_HANDLE) {
            destroy_render_pass(state.device, state.fb_render_pass);
        }

        if (state.command_pool != VK_NULL_HANDLE) {
            destroy_command_pool(state.device, state.command_pool);
        }

        for (const auto &texture : state.prepared_textures) {
            destroy_texture(state.device, texture.second.value);
        }

        destroy_swapchain(state, state.swapchain);

        vkDestroySurfaceKHR(g_vk_instance, state.surface, nullptr);
    }

    void VulkanRenderer::init(void) {
        state.swapchain = create_swapchain(this->state, state.surface, this->window.peek_resolution());
        Logger::default_logger().debug("Created swapchain for new window");

        state.composite_pipeline = create_pipeline(this->state, { FB_SHADER_VERT_PATH, FB_SHADER_FRAG_PATH },
                this->state.swapchain.composite_render_pass);
        Logger::default_logger().debug("Created composite pipeline");

        state.composite_vbo = alloc_buffer(state.device, sizeof(g_frame_quad_vertex_data),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                     | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        auto *mapped_comp_vbo = map_buffer(state.device, state.composite_vbo, 0, sizeof(g_frame_quad_vertex_data), 0);
        memcpy(mapped_comp_vbo, g_frame_quad_vertex_data, sizeof(g_frame_quad_vertex_data));
        unmap_buffer(state.device, state.composite_vbo);
        Logger::default_logger().debug("Created composite VBO");

        state.fb_render_pass = create_render_pass(this->state.device, this->state.swapchain.image_format,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        Logger::default_logger().debug("Created framebuffer render pass for new window");

        is_initted = true;
    }

    void VulkanRenderer::render(TimeDelta delta) {
        UNUSED(delta);

        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> timer_start;
        std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> timer_end;

        auto vsync = window.is_vsync_enabled();
        if (vsync.dirty) {
            //glfwSwapInterval(vsync ? 1 : 0);
        }

        timer_start = std::chrono::high_resolution_clock::now();
        _rebuild_scene(window, state);
        timer_end = std::chrono::high_resolution_clock::now();
        //printf("Rebuilding scenes took %ld ns\n", (timer_end - timer_start).count());

        auto &canvas = window.get_canvas();

        auto resolution = window.get_resolution();

        for (auto &viewport_state : state.viewport_states_2d) {
            viewport_state.second.visited = false;
        }

        auto viewports = canvas.get_viewports_2d();
        std::sort(viewports.begin(), viewports.end(),
                [](auto a, auto b) { return a.get().get_z_index() < b.get().get_z_index(); });

        timer_start = std::chrono::high_resolution_clock::now();
        for (auto viewport : viewports) {
            auto &viewport_state = state.get_viewport_state(viewport);
            auto &scene = viewport.get().get_camera().get_scene();
            auto &scene_state = state.get_scene_state(scene);

            viewport_state.visited = true;

            draw_scene_to_framebuffer(scene_state, viewport_state, resolution);
        }
        timer_end = std::chrono::high_resolution_clock::now();
        //printf("Drawing scenes took %ld ns\n", (timer_end - timer_start).count());

        std::vector<const AttachedViewport2D *> viewports_to_remove;
        for (auto it = state.viewport_states_2d.begin(); it != state.viewport_states_2d.end();) {
            if (!it->second.visited) {
                _destroy_viewport(state, it->second);
                it = state.viewport_states_2d.erase(it);
            } else {
                it++;
            }
        }

        // set up state for drawing framebuffers to screen

        //glClearColor(0.0, 0.0, 0.0, 1.0);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        timer_start = std::chrono::high_resolution_clock::now();

        auto image_index = _get_next_image(state);
        auto sc_image = state.swapchain.images[image_index];

        vkResetCommandBuffer(state.draw_cmd_buf.handle, 0);

        VkCommandBufferBeginInfo cmd_begin_info{};
        cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_begin_info.flags = 0;
        vkBeginCommandBuffer(state.draw_cmd_buf.handle, &cmd_begin_info);

        perform_image_transition(state.draw_cmd_buf, sc_image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        auto vk_cmd_buf = state.draw_cmd_buf.handle;

        auto fb_width = state.swapchain.extent.width;
        auto fb_height = state.swapchain.extent.height;

        VkClearValue clear_val{};
        clear_val.color = { { 0.0, 0.0, 0.0, 0.0 } };

        VkRenderPassBeginInfo rp_info{};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_info.framebuffer = state.swapchain.framebuffers[image_index];
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

            draw_framebuffer_to_swapchain(scene_state, viewport_state);
        }

        vkCmdEndRenderPass(vk_cmd_buf);

        /*perform_image_transition(state.draw_cmd_buf, sc_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);*/

        vkEndCommandBuffer(state.draw_cmd_buf.handle);

        _submit_queues(state);
        timer_end = std::chrono::high_resolution_clock::now();
        //printf("Compositing scenes took %ld ns\n", (timer_end - timer_start).count());
        _present_image(state, image_index);

        //glfwSwapBuffers(get_window_handle<GLFWwindow>(canvas.get_window()));
    }

    void VulkanRenderer::notify_window_resize(const Vector2u &resolution) {
        recreate_swapchain(state, resolution, state.swapchain);
        _update_view_matrix(this->window, this->state, resolution);
    }
}

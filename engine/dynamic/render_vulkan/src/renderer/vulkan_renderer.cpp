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

#include "argus/render/common/canvas.hpp"

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
#include "internal/render_vulkan/util/pipeline.hpp"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"
#include "argus/render/defines.hpp"

namespace argus {// forward declarations
    class Scene2D;

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
                0,                              2 / (float(t - b) * ver_scale), 0, -float(t + b) /
                                                                                    (float(t - b) * ver_scale),
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

    /*static void _update_view_matrix(const Window &window, RendererState &state, const Vector2u &resolution) {
        auto &canvas = window.get_canvas();

        for (auto viewport : canvas.get_viewports_2d()) {
            auto &viewport_state
                    = reinterpret_cast<Viewport2DState &>(state.get_viewport_state(viewport, true));
            auto camera_transform = viewport.get().get_camera().peek_transform();
            _recompute_2d_viewport_view_matrix(viewport_state.viewport->get_viewport(), camera_transform.inverse(),
                    resolution,
                    viewport_state.view_matrix);
        }
    }*/

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
    }

    VulkanRenderer::VulkanRenderer(Window &window):
        window(window) {
        UNUSED(this->resource_event_handler);

        state.device = g_vk_device;

        auto surface_err = glfwCreateWindowSurface(g_vk_instance,
                get_window_handle<GLFWwindow>(window), nullptr, &state.surface);
        if (surface_err) {
            Logger::default_logger().fatal("glfwCreateWindowSurface returned error code %d", surface_err);
        }
        Logger::default_logger().debug("Created surface for new window");

        state.swapchain = create_vk_swapchain(this->window, this->state.device, state.surface);
        Logger::default_logger().debug("Created swapchain for new window");

        state.render_pass = create_render_pass(this->state.device, this->state.swapchain.image_format);
        Logger::default_logger().debug("Created render pass for new window");

        state.command_pool = create_command_pool(this->state.device);
        Logger::default_logger().debug("Created command pool for new window");

        state.desc_pool = create_descriptor_pool(this->state.device);
        Logger::default_logger().debug("Created descriptor pool for new window");

        state.global_ubo = alloc_buffer(this->state.device, SHADER_UBO_GLOBAL_LEN, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    VulkanRenderer::~VulkanRenderer(void) {
        vkDestroySwapchainKHR(g_vk_device.logical_device, state.swapchain.swapchain, nullptr);
        vkDestroySurfaceKHR(g_vk_instance, state.surface, nullptr);
    }

    void VulkanRenderer::render(TimeDelta delta) {
        UNUSED(delta);

        auto vsync = window.is_vsync_enabled();
        if (vsync.dirty) {
            //glfwSwapInterval(vsync ? 1 : 0);
        }

        _rebuild_scene(window, state);

        auto &canvas = window.get_canvas();

        auto resolution = window.get_resolution();

        auto viewports = canvas.get_viewports_2d();
        std::sort(viewports.begin(), viewports.end(),
                [](auto a, auto b) { return a.get().get_z_index() < b.get().get_z_index(); });

        for (auto viewport : viewports) {
            auto &viewport_state = state.get_viewport_state(viewport);
            auto &scene = viewport.get().get_camera().get_scene();
            auto &scene_state = state.get_scene_state(scene);
            draw_scene_to_framebuffer(scene_state, viewport_state, resolution);
        }

        // set up state for drawing framebuffers to screen

        //glClearColor(0.0, 0.0, 0.0, 1.0);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto &viewport : viewports) {
            auto &viewport_state = state.get_viewport_state(viewport);
            auto &scene = viewport.get().get_camera().get_scene();
            auto &scene_state = state.get_scene_state(scene);

            draw_framebuffer_to_screen(scene_state, viewport_state, resolution);
        }

        //glfwSwapBuffers(get_window_handle<GLFWwindow>(canvas.get_window()));
    }

    void VulkanRenderer::notify_window_resize(const Vector2u &resolution) {
        UNUSED(resolution);
        //TODO
    }
}

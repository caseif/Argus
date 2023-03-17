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

#include "argus/wm/window.hpp"

#include "internal/render_vulkan/module_render_vulkan.hpp"
#include "internal/render_vulkan/renderer/vulkan_renderer.hpp"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

namespace argus {
    VulkanRenderer::VulkanRenderer(Window &window):
        window(window) {
        auto surface_err = glfwCreateWindowSurface(g_vk_instance,
                get_window_handle<GLFWwindow>(window), nullptr, &this->surface);

        if (surface_err) {
            Logger::default_logger().fatal("glfwCreateWindowSurface returned error code %d", surface_err);
        }

        Logger::default_logger().debug("Created surface for new window");
    }

    VulkanRenderer::~VulkanRenderer(void) {
        vkDestroySurfaceKHR(g_vk_instance, this->surface, nullptr);
    }

    void VulkanRenderer::render(TimeDelta delta) {
        UNUSED(delta);
        //TODO
    }

    void VulkanRenderer::notify_window_resize(const Vector2u &resolution) {
        UNUSED(resolution);
        //TODO
    }
}

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
#include "argus/lowlevel/macros.hpp"

#include "argus/core/engine.hpp"
#include "argus/core/module.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/backend.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/loader/shader_loader.hpp"
#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/setup/instance.hpp"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include <map>
#include <string>

#include <cassert>
#include <cstring>

namespace argus {
    static bool g_backend_active = false;

    static VkInstance g_vk_instance = nullptr;
    static std::map<const Window *, VkSurfaceKHR> g_surface_map;

    static bool _test_vulkan_support() {
        //TODO: eventually we'll check the available extensions and verify we can actually use Vulkan to render
        return glfwVulkanSupported() == GLFW_TRUE;
    }

    static bool _activate_vulkan_backend() {
        if (!_test_vulkan_support()) {
            return false;
        }

        g_backend_active = true;
        return true;
    }

    static void _window_event_callback(const WindowEvent &event, void *user_data) {
        UNUSED(user_data);
        const Window &window = event.window;

        switch (event.subtype) {
            case WindowEventType::Create: {
                VkSurfaceKHR surface;
                auto surface_err = glfwCreateWindowSurface(g_vk_instance,
                        get_window_handle<GLFWwindow>(window), nullptr, &surface);

                if (surface_err) {
                    Logger::default_logger().fatal("glfwCreateWindowSurface returned value %d", surface_err);
                }
                //TODO: store the surface

                //auto *renderer = new GLRenderer(window);
                //g_renderer_map.insert({ &window, renderer });
                break;
            }
            case WindowEventType::Update: {
                if (!window.is_ready()) {
                    return;
                }

                //auto it = g_renderer_map.find(&window);
                //assert(it != g_renderer_map.end());

                //it->second->render(event.delta);
                break;
            }
            case WindowEventType::Resize: {
                if (!window.is_ready()) {
                    return;
                }

                //auto it = g_renderer_map.find(&window);
                //assert(it != g_renderer_map.end());

                //it->second->notify_window_resize(event.resolution);
                break;
            }
            case WindowEventType::RequestClose: {
                //auto it = g_renderer_map.find(&window);
                //assert(it != g_renderer_map.end());

                //delete it->second;
                break;
            }
            default: {
                break;
            }
        }
    }

    void update_lifecycle_render_vulkan(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit: {
                register_render_backend(BACKEND_ID, _activate_vulkan_backend);

                break;
            }
            case LifecycleStage::Init: {
                if (!g_backend_active) {
                    return;
                }

                ResourceManager::instance().register_loader(*new ShaderLoader());

                register_event_handler<WindowEvent>(_window_event_callback, TargetThread::Render);

                g_vk_instance = create_vk_instance();

                auto vk_dev = create_vk_device(g_vk_instance);
                UNUSED(vk_dev);

                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

                break;
            }
            case LifecycleStage::PostInit: {
                if (!g_backend_active) {
                    return;
                }

                /*ResourceManager::instance().add_memory_package(RESOURCES_RENDER_OPENGL_ARP_SRC,
                        RESOURCES_RENDER_OPENGL_ARP_LEN);*/
                break;
            }
            default: {
                break;
            }
        }
    }

    REGISTER_ARGUS_MODULE("render_vulkan", update_lifecycle_render_vulkan, { "render" });
}

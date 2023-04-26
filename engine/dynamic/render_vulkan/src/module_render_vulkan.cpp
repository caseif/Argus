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

#include "argus/core/module.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/backend.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/module_render_vulkan.hpp"
#include "internal/render_vulkan/resources.h"
#include "internal/render_vulkan/loader/shader_loader.hpp"
#include "internal/render_vulkan/renderer/vulkan_renderer.hpp"
#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/setup/instance.hpp"

#pragma GCC diagnostic push

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdocumentation"
#endif
#include "GLFW/glfw3.h"
#pragma GCC diagnostic pop
#include "vulkan/vulkan.h"

#include <map>
#include <vector>

namespace argus {
    static bool g_backend_active = false;

    std::vector<const char *> g_engine_device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME
    };

    std::vector<const char *> g_engine_instance_extensions = {
            #ifdef _ARGUS_DEBUG_MODE
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            #endif
    };

    std::vector<const char *> g_engine_layers = {
            #ifdef _ARGUS_DEBUG_MODE
            "VK_LAYER_KHRONOS_validation"
            #endif
    };

    VkInstance g_vk_instance = nullptr;
    LogicalDevice g_vk_device{};
    VkDebugUtilsMessengerEXT g_vk_debug_messenger;

    static Logger g_vk_logger("Vulkan");

    static std::map<const Window *, VulkanRenderer *> g_renderer_map;

    VKAPI_ATTR static VkBool32 VKAPI_CALL _debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
            void* user_data) {
        UNUSED(type);
        UNUSED(user_data);

        char const *level;
        bool is_error = false;
        switch (severity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                level = "SEVERE";
                is_error = true;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                level = "WARN";
                is_error = true;
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                level = "INFO";
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                level = "TRACE";
                break;
            default: // shouldn't happen
                level = "UNKNOWN";
                is_error = true;
                break;
        }
        if (is_error) {
            g_vk_logger.log_error(level, "%s", callback_data->pMessage);
        } else {
            g_vk_logger.log(level, "%s", callback_data->pMessage);
        }
        return true;
    }

    static bool _activate_vulkan_backend() {
        if (glfwVulkanSupported() != GLFW_TRUE) {
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        auto vk_inst = create_vk_instance();
        if (!vk_inst.has_value()) {
            return false;
        }
        g_vk_instance = vk_inst.value();

        #ifdef _ARGUS_DEBUG_MODE
        VkDebugUtilsMessengerCreateInfoEXT debug_info{};
        debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debug_info.pfnUserCallback = _debug_callback;

        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT
                = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vk_inst.value(),
                        "vkCreateDebugUtilsMessengerEXT"));

        vkCreateDebugUtilsMessengerEXT(vk_inst.value(), &debug_info, nullptr, &g_vk_debug_messenger);
        #else
        UNUSED(_debug_callback);
        #endif

        // create hidden window so we can attach a surface and probe for capabilities
        auto *window = glfwCreateWindow(1, 1, "", nullptr, nullptr);

        if (window == nullptr) {
            Logger::default_logger().warn("Failed to probe Vulkan capabilities (glfwCreateWindow failed)");
            return false;
        }

        VkSurfaceKHR probe_surface{};
        glfwCreateWindowSurface(g_vk_instance, window, nullptr, &probe_surface);

        auto vk_device = create_vk_device(g_vk_instance, probe_surface);

        vkDestroySurfaceKHR(g_vk_instance, probe_surface, nullptr);
        glfwDestroyWindow(window);

        if (!vk_device.has_value()) {
            destroy_vk_instance(g_vk_instance);
            return false;
        }
        g_vk_device = vk_device.value();

        g_backend_active = true;
        return true;
    }

    static void _window_event_callback(const WindowEvent &event, void *user_data) {
        UNUSED(user_data);
        Window &window = event.window;

        switch (event.subtype) {
            case WindowEventType::Create: {
                auto *renderer = new VulkanRenderer(window);
                g_renderer_map.insert({&window, renderer});
                break;
            }
            case WindowEventType::Update: {
                if (!window.is_ready()) {
                    return;
                }

                auto it = g_renderer_map.find(&window);
                assert(it != g_renderer_map.end());
                auto &renderer = it->second;

                if (!renderer->is_initted) {
                    renderer->init();
                }

                renderer->render(event.delta);
                break;
            }
            case WindowEventType::Resize: {
                if (!window.is_ready()) {
                    return;
                }

                auto it = g_renderer_map.find(&window);
                assert(it != g_renderer_map.end());

                it->second->notify_window_resize(event.resolution);
                break;
            }
            case WindowEventType::RequestClose: {
                auto it = g_renderer_map.find(&window);
                assert(it != g_renderer_map.end());

                delete it->second;
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

                break;
            }
            case LifecycleStage::PostInit: {
                if (!g_backend_active) {
                    return;
                }

                ResourceManager::instance().add_memory_package(RESOURCES_RENDER_VULKAN_ARP_SRC,
                        RESOURCES_RENDER_VULKAN_ARP_LEN);
                break;
            }
            case LifecycleStage::Deinit: {
                destroy_vk_device(g_vk_device);

                destroy_vk_instance(g_vk_instance);

                break;
            }
            default: {
                break;
            }
        }
    }

    REGISTER_ARGUS_MODULE("render_vulkan", update_lifecycle_render_vulkan, { "render" })
}

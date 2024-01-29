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

#include "argus/wm/api_util.hpp"
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
            VK_KHR_SURFACE_EXTENSION_NAME,
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

    [[maybe_unused]] VKAPI_ATTR static VkBool32 VKAPI_CALL _debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
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

    static void _init_vk_debug_utils(VkInstance inst) {
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
                = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(inst,
                        "vkCreateDebugUtilsMessengerEXT"));

        vkCreateDebugUtilsMessengerEXT(inst, &debug_info, nullptr, &g_vk_debug_messenger);
        #else
        UNUSED(inst);
        #endif
    }

    static void _deinit_vk_debug_utils(VkInstance inst) {
        #ifdef _ARGUS_DEBUG_MODE
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT
                = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(inst,
                        "vkDestroyDebugUtilsMessengerEXT"));
        vkDestroyDebugUtilsMessengerEXT(inst, g_vk_debug_messenger, nullptr);
        #else
        UNUSED(inst);
        #endif
    }

    static bool _activate_vulkan_backend() {
        set_window_creation_flags(WindowCreationFlags::Vulkan);

        if (!vk_is_supported()) {
            set_window_creation_flags(WindowCreationFlags::None);
            return false;
        }

        // create hidden window so we can attach a surface and probe for capabilities
        auto &window = Window::create("", nullptr);
        window.update({});

        auto vk_inst = create_vk_instance(window);
        if (!vk_inst.has_value()) {
            set_window_creation_flags(WindowCreationFlags::None);
            return false;
        }
        g_vk_instance = vk_inst.value();

        _init_vk_debug_utils(vk_inst.value());

        /*if (window == nullptr) {
            _deinit_vk_debug_utils(g_vk_instance);
            destroy_vk_instance(g_vk_instance);
            Logger::default_logger().warn("Failed to probe Vulkan capabilities (window creation failed)");
            return false;
        }*/

        VkSurfaceKHR probe_surface{};
        if (!vk_create_surface(window, reinterpret_cast<void *>(g_vk_instance),
                reinterpret_cast<void **>(&probe_surface))) {
            Logger::default_logger().warn("Vulkan does not appear to be supported (failed to create surface)");
            set_window_creation_flags(WindowCreationFlags::None);
            return false;
        }

        auto vk_device = create_vk_device(g_vk_instance, probe_surface);

        vkDestroySurfaceKHR(g_vk_instance, probe_surface, nullptr);
        window.request_close();

        if (!vk_device.has_value()) {
            Logger::default_logger().info("Vulkan does not appear to be supported (could not get Vulkan device)");
            _deinit_vk_debug_utils(vk_inst.value());
            destroy_vk_instance(g_vk_instance);
            set_window_creation_flags(WindowCreationFlags::None);
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

    extern "C" {
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
                if (!g_backend_active) {
                    return;
                }

                destroy_vk_device(g_vk_device);
                _deinit_vk_debug_utils(g_vk_instance);
                destroy_vk_instance(g_vk_instance);

                break;
            }
            default: {
                break;
            }
        }
    }
    }

    REGISTER_ARGUS_MODULE("render_vulkan", update_lifecycle_render_vulkan, { "render" })
}

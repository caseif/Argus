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
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"

#include "argus/core/client_properties.hpp"
#include "argus/core/macros.hpp"

#include "internal/render_vulkan/setup/instance.hpp"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include <algorithm>
#include <string>
#include <vector>

#include <cstdint>
#include <cstring>

namespace argus {
    static const std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    static std::vector<VkExtensionProperties> _get_available_extensions(void) {
        uint32_t ext_count;
        vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);

        std::vector<VkExtensionProperties> exts(ext_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, exts.data());

        return exts;
    }

    static std::vector<VkLayerProperties> _get_available_layers(void) {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        return available_layers;
    }

    static bool _check_required_extensions(const char *const *extensions, uint32_t extensions_count) {
        auto available_exts = _get_available_extensions();

        for (uint32_t i = 0; i < extensions_count; i++) {
            auto ext_name = extensions[i];
            auto ext_it = std::find_if(available_exts.begin(), available_exts.end(),
                    [ext_name](auto ext) { return strcmp(ext.extensionName, ext_name) == 0; });
            if (ext_it == available_exts.end()) {
                Logger::default_logger().warn("Extension '%s' is not available (required by GLFW)", ext_name);
                return false;
            }
        }

        return true;
    }

    static bool _check_required_layers(const char *const *layers, uint32_t layers_count) {
        #ifdef _ARGUS_DEBUG_MODE
            return true;
        #endif

        auto available_layers = _get_available_layers();

        for (uint32_t i = 0; i < layers_count; i++) {
            auto layer_name = layers[i];
            if (std::none_of(available_layers.begin(), available_layers.end(),
                    [layer_name](auto layer) { return strcmp(layer.layerName, layer_name) == 0; })) {
                Logger::default_logger().warn("Validation layer '%s' is not available", layer_name);
                return false;
            }
        }

        return true;
    }

    static VkInstance _create_instance(const char *const *extensions, uint32_t extensions_count,
            const char *const *validation_layers, uint32_t validation_layers_count) {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = get_client_name().c_str();
        app_info.pEngineName = ENGINE_NAME;
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wold-style-cast"

        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0); //TODO: parse out client version into three components
        app_info.engineVersion = VK_MAKE_API_VERSION(0, ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_INCR);
        app_info.apiVersion = VK_API_VERSION_1_0;

        #pragma GCC diagnostic pop

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        create_info.enabledLayerCount = validation_layers_count;
        create_info.ppEnabledLayerNames = validation_layers;

        create_info.enabledExtensionCount = extensions_count;
        create_info.ppEnabledExtensionNames = extensions;

        create_info.enabledLayerCount = validation_layers_count;
        create_info.ppEnabledLayerNames = validation_layers;

        VkInstance instance;

        auto createRes = vkCreateInstance(&create_info, nullptr, &instance);
        _ARGUS_ASSERT_F(createRes == VK_SUCCESS, "vkCreateInstance returned error code %d", createRes);

        return instance;
    }

    VkInstance create_vk_instance(void) {
        uint32_t glfw_exts_count;
        auto glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_exts_count);

        if (!_check_required_extensions(glfw_exts, glfw_exts_count)) {
            Logger::default_logger().fatal("Required Vulkan extensions for GLFW are not available");
        }

        const char *const *layers;
        uint32_t layers_count;
        #ifdef _ARGUS_DEBUG_MODE
            layers = validation_layers.data();
            layers_count = static_cast<uint32_t>(validation_layers.size());
        #else
            layers = nullptr;
            layers_count = 0;
        #endif

        if (!_check_required_layers(layers, layers_count)) {
            Logger::default_logger().warn("Required Vulkan extensions for GLFW are not available");
            layers = nullptr;
            layers_count = 0;
        }

        auto instance = _create_instance(glfw_exts, glfw_exts_count, layers, layers_count);

        return instance;
    }
}

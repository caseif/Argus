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

#include "argus/core/client_properties.hpp"
#include "argus/core/macros.hpp"

#include "internal/render_vulkan/module_render_vulkan.hpp"
#include "internal/render_vulkan/setup/instance.hpp"

#include "vulkan/vulkan.h"
#include "argus/wm/api_util.hpp"

#include <algorithm>
#include <optional>
#include <vector>

#include <cstring>

namespace argus {
    static std::vector<VkExtensionProperties> _get_available_extensions(void) {
        uint32_t ext_count;
        vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);

        std::vector<VkExtensionProperties> exts(ext_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, exts.data());

        return exts;
    }

    [[maybe_unused]] static std::vector<VkLayerProperties> _get_available_layers(void) {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        return available_layers;
    }

    static bool _check_required_extensions(const std::vector<const char *> &exts) {
        auto available_exts = _get_available_extensions();

        for (const auto &ext_name : exts) {
            auto ext_it = std::find_if(available_exts.begin(), available_exts.end(),
                    [ext_name](auto ext) { return strcmp(ext.extensionName, ext_name) == 0; });
            if (ext_it == available_exts.end()) {
                Logger::default_logger().warn("Required Vulkan extension '%s' is not available", ext_name);
                return false;
            }
        }

        return true;
    }

    static bool _check_required_layers(const std::vector<const char *> &layers) {
        #ifdef _ARGUS_DEBUG_MODE
        auto available_layers = _get_available_layers();

        for (const auto &layer_name : layers) {
            if (std::none_of(available_layers.begin(), available_layers.end(),
                    [layer_name](auto layer) { return strcmp(layer.layerName, layer_name) == 0; })) {
                Logger::default_logger().warn("Required Vulkan layer '%s' is not available", layer_name);
                return false;
            }
        }
        #else
        UNUSED(layers);
        #endif

        return true;
    }

    static VkInstance _create_instance(const std::vector<const char *> &extensions,
            const std::vector<const char *> &layers) {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = get_client_name().c_str();
        app_info.pEngineName = ARGUS_ENGINE_NAME;

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wold-style-cast"

        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0,
                0); //TODO: parse out client version into three components
        app_info.engineVersion = VK_MAKE_API_VERSION(0, ARGUS_ENGINE_VERSION_MAJOR, ARGUS_ENGINE_VERSION_MINOR,
                ARGUS_ENGINE_VERSION_INCR);
        app_info.apiVersion = VK_API_VERSION_1_0;

        #pragma GCC diagnostic pop

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        create_info.enabledLayerCount = uint32_t(layers.size());
        create_info.ppEnabledLayerNames = layers.data();

        create_info.enabledExtensionCount = uint32_t(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        VkInstance instance;

        auto createRes = vkCreateInstance(&create_info, nullptr, &instance);
        if (createRes != VK_SUCCESS) {
            Logger::default_logger().warn("vkCreateInstance returned error code %d", createRes);
            return nullptr;
        }

        return instance;
    }

    std::optional<VkInstance> create_vk_instance(Window &window) {
        uint32_t req_exts_count;
        std::vector<const char *> req_exts_names;
        if (!vk_get_required_instance_extensions(window, &req_exts_count, nullptr)) {
            Logger::default_logger().warn("Failed to get required instance extensions");
        }
        req_exts_names.resize(req_exts_count);
        if (!vk_get_required_instance_extensions(window, &req_exts_count, req_exts_names.data())) {
            Logger::default_logger().warn("Failed to get required instance extensions");
        }
        std::vector<const char *> all_exts;
        for (size_t i = 0; i < req_exts_count; i++) {
            all_exts.push_back(req_exts_names[i]);
        }

        all_exts.insert(all_exts.end(), g_engine_instance_extensions.cbegin(), g_engine_instance_extensions.cend());

        if (!_check_required_extensions(all_exts)) {
            Logger::default_logger().warn("Required Vulkan extensions are not available");
            return std::nullopt;
        }

        std::vector<const char *> all_layers;
        #ifdef _ARGUS_DEBUG_MODE
        all_layers.insert(all_layers.begin(), g_engine_layers.cbegin(), g_engine_layers.cend());
        #endif

        if (!_check_required_layers(all_layers)) {
            Logger::default_logger().warn("Required Vulkan extensions for engine are not available");
            return std::nullopt;
        }

        auto instance = _create_instance(all_exts, all_layers);
        if (instance == nullptr) {
            return std::nullopt;
        }

        return std::make_optional(instance);
    }

    void destroy_vk_instance(VkInstance instance) {
        vkDestroyInstance(instance, nullptr);
    }
}

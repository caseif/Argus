/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/macros.hpp"
#include "internal/lowlevel/logging.hpp"

#include "argus/core/macros.hpp"

#include "internal/render_vulkan/instance.hpp"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"

#include <algorithm>
#include <vector>

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

    static bool _check_required_glfw_extensions(std::vector<VkExtensionProperties> &available_exts) {
        uint32_t glfw_ext_count;
        auto glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

        for (uint32_t i = 0; i < glfw_ext_count; i++) {
            auto glfw_ext = glfw_exts[i];
            if (std::none_of(available_exts.begin(), available_exts.end(),
                    [glfw_ext](auto ext) { return strcmp(ext.extensionName, glfw_ext) == 0; })) {
                return false;
            }
        }

        return true;
    }

    static bool _check_required_validation_layers() {
        if (_ARGUS_DEBUG_MODE) {
            return true;
        }

        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (uint32_t i = 0; i < layer_count; i++) {
            auto layer = validation_layers.at(i);
            if (std::none_of(available_layers.begin(), available_layers.end(),
                    [layer](auto ext) { return strcmp(ext.layerName, layer) == 0; })) {
                return false;
            }
        }
    }

    static VkInstance _create_instance(void) {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Argus Game"; //TODO: use the client application name eventually
        app_info.pEngineName = ENGINE_NAME;
        
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wold-style-cast"
        
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0); //TODO: same thing
        app_info.engineVersion = VK_MAKE_API_VERSION(0, ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_INCR);
        app_info.apiVersion = VK_API_VERSION_1_0;
        
        #pragma GCC diagnostic pop

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledLayerCount = 0; //TODO

        if (_ARGUS_DEBUG_MODE) {
            if (_check_required_validation_layers()) {
                create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
                create_info.ppEnabledLayerNames = validation_layers.data();
            } else {
                _ARGUS_WARN("Vulkan validation layers are not available");
                create_info.enabledLayerCount = 0;
            }
        } else {
            create_info.enabledLayerCount = 0;
        }

        VkInstance instance;

        auto createRes = vkCreateInstance(&create_info, nullptr, &instance);
        _ARGUS_ASSERT(createRes == VK_SUCCESS, "vkCreateInstance returned error code %d", createRes);

        return instance;
    }

    VkInstance create_and_init_vk_instance(void) {
        auto available_exts = _get_available_extensions();

        _ARGUS_ASSERT(_check_required_glfw_extensions(available_exts),
                "Required Vulkan extensions for GLFW are not available");

        auto instance = _create_instance();

        return instance;
    }
}

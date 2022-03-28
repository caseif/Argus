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

#include "internal/render_vulkan/setup/device_physical.hpp"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include <cstdint>

namespace argus {
    static const uint32_t DISCRETE_GPU_RATING_BONUS = 10000;

    static bool _is_queue_family_suitable(VkInstance instance, VkPhysicalDevice device,
            VkQueueFamilyProperties queue_family) {
        if (glfwGetPhysicalDevicePresentationSupport(instance, device, queue_family.queueCount) == GLFW_FALSE) {
            return false;
        }

        if (!(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            return false;
        }

        return true;
    }

    static bool _is_physical_device_suitable(VkInstance instance, VkPhysicalDevice device,
            std::vector<VkQueueFamilyProperties> queue_families) {
        for (auto queue_family : queue_families) {
            if (_is_queue_family_suitable(instance, device, queue_family)) {
                return true;
            }
        }

        return false;
    }

    static uint32_t _rate_physical_device(VkInstance instance, VkPhysicalDevice device,
            std::vector<VkQueueFamilyProperties> queue_families) {
        UNUSED(instance);
        UNUSED(queue_families);

        uint32_t score = 0;

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += DISCRETE_GPU_RATING_BONUS;
        }

        score += props.limits.maxImageDimension2D;

        //TODO: do some more checks at some point

        return score;
    }

    static VkPhysicalDevice _select_physical_device(VkInstance instance) {
        uint32_t dev_count = 0;
        auto enum_res = vkEnumeratePhysicalDevices(instance, &dev_count, nullptr);
        if (enum_res) {
            _ARGUS_FATAL("vkEnumeratePhysicalDevices returned error code %d", enum_res);
        }

        _ARGUS_ASSERT(dev_count > 0, "No physical video devices found");

        std::vector<VkPhysicalDevice> devs(dev_count);
        enum_res = vkEnumeratePhysicalDevices(instance, &dev_count, devs.data());

        VkPhysicalDevice best_dev = nullptr;
        uint32_t best_rating = 0;
        for (auto dev : devs) {
            VkPhysicalDeviceProperties dev_props;
            vkGetPhysicalDeviceProperties(dev, &dev_props);

            _ARGUS_DEBUG("Considering physical device '%s'", dev_props.deviceName);

            uint32_t qf_props_count;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &qf_props_count, nullptr);
            if (qf_props_count == 0) {
                _ARGUS_DEBUG("Physical device '%s' has no queue families", dev_props.deviceName);
                continue;
            }

            std::vector<VkQueueFamilyProperties> queue_families(qf_props_count);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &qf_props_count, queue_families.data());

            if (!_is_physical_device_suitable(instance, dev, queue_families)) {
                _ARGUS_DEBUG("Physical device '%s' is not suitable", dev_props.deviceName);
                continue;
            }

            auto rating = _rate_physical_device(instance, dev, queue_families);
            _ARGUS_DEBUG("Physical device '%s' was assigned rating of %d", dev_props.deviceName, rating);
            if (rating > best_rating) {
                best_dev = dev;
                best_rating = rating;
            }
        }
        
        if (best_rating == 0) {
            _ARGUS_FATAL("Failed to find suitable video device");
        }

        return best_dev;
    }

    VkDevice create_vk_device(VkInstance instance) {
        auto phys_dev = _select_physical_device(instance);
        VkPhysicalDeviceProperties phys_dev_props;
        vkGetPhysicalDeviceProperties(phys_dev, &phys_dev_props);

        _ARGUS_INFO("Selected video device %s", phys_dev_props.deviceName);

        return {};
    }
}
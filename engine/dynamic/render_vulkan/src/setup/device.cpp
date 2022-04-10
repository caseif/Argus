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
#include "argus/lowlevel/optional.hpp"
#include "internal/lowlevel/logging.hpp"

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/setup/queues.hpp"
#include "vulkan/vulkan_core.h"

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

    static Optional<QueueFamilyIndices> _get_queue_family_indices(VkInstance instance, VkPhysicalDevice device,
            std::vector<VkQueueFamilyProperties> queue_families) {
        QueueFamilyIndices indices = {};

        uint32_t i = 0;
        for (auto queue_family : queue_families) {
            // check if queue family is suitable for graphics
            if (glfwGetPhysicalDevicePresentationSupport(instance, device, i) == GLFW_TRUE) {
                if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphics_family = i;
                }
            }

            i++;
        }

        return indices;
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

    static std::pair<VkPhysicalDevice, QueueFamilyIndices> _select_physical_device(VkInstance instance) {
        uint32_t dev_count = 0;
        auto enum_res = vkEnumeratePhysicalDevices(instance, &dev_count, nullptr);
        if (enum_res) {
            _ARGUS_FATAL("vkEnumeratePhysicalDevices returned error code %d", enum_res);
        }

        _ARGUS_ASSERT(dev_count > 0, "No physical video devices found");

        std::vector<VkPhysicalDevice> devs(dev_count);
        enum_res = vkEnumeratePhysicalDevices(instance, &dev_count, devs.data());

        VkPhysicalDevice best_dev = nullptr;
        QueueFamilyIndices best_dev_indices;
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

            auto indices = _get_queue_family_indices(instance, dev, queue_families);
            if (!indices.has_value()) {
                _ARGUS_DEBUG("Physical device '%s' is not suitable", dev_props.deviceName);
                continue;
            }

            auto rating = _rate_physical_device(instance, dev, queue_families);
            _ARGUS_DEBUG("Physical device '%s' was assigned rating of %d", dev_props.deviceName, rating);
            if (rating > best_rating) {
                best_dev = dev;
                best_dev_indices = indices;
                best_rating = rating;
            }
        }

        if (best_rating == 0) {
            _ARGUS_FATAL("Failed to find suitable video device");
        }

        return std::make_pair(best_dev, best_dev_indices);
    }

    VkDevice create_vk_device(VkInstance instance) {
        VkPhysicalDevice phys_dev;
        QueueFamilyIndices qf_indices;
        std::tie(phys_dev, qf_indices) = _select_physical_device(instance);
        VkPhysicalDeviceProperties phys_dev_props;
        vkGetPhysicalDeviceProperties(phys_dev, &phys_dev_props);

        _ARGUS_INFO("Selected video device %s", phys_dev_props.deviceName);

        return {};
    }
}
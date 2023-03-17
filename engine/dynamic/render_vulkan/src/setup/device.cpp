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

#include "internal/render_vulkan/module_render_vulkan.hpp"
#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/setup/queues.hpp"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

#include <algorithm>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include <cstdint>

namespace argus {
    static const uint32_t DISCRETE_GPU_RATING_BONUS = 10000;

    static std::optional<QueueFamilyIndices> _get_queue_family_indices(VkInstance instance, VkPhysicalDevice device,
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
            Logger::default_logger().fatal("vkEnumeratePhysicalDevices returned error code %d", enum_res);
        }

        affirm_precond(dev_count > 0, "No physical video devices found");

        std::vector<VkPhysicalDevice> devs(dev_count);
        enum_res = vkEnumeratePhysicalDevices(instance, &dev_count, devs.data());

        VkPhysicalDevice best_dev = nullptr;
        QueueFamilyIndices best_dev_indices;
        uint32_t best_rating = 0;

        for (auto dev : devs) {
            VkPhysicalDeviceProperties dev_props;
            vkGetPhysicalDeviceProperties(dev, &dev_props);

            Logger::default_logger().debug("Considering physical device '%s'", dev_props.deviceName);

            uint32_t qf_props_count;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &qf_props_count, nullptr);
            if (qf_props_count == 0) {
                Logger::default_logger().debug("Physical device '%s' has no queue families", dev_props.deviceName);
                continue;
            }

            std::vector<VkQueueFamilyProperties> queue_families(qf_props_count);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &qf_props_count, queue_families.data());

            auto indices = _get_queue_family_indices(instance, dev, queue_families);
            if (!indices.has_value()) {
                Logger::default_logger().debug("Physical device '%s' is not suitable", dev_props.deviceName);
                continue;
            }

            auto rating = _rate_physical_device(instance, dev, queue_families);
            Logger::default_logger().debug("Physical device '%s' was assigned rating of %d", dev_props.deviceName,
                    rating);
            if (rating > best_rating) {
                best_dev = dev;
                best_dev_indices = indices.value();
                best_rating = rating;
            }
        }

        if (best_rating == 0) {
            Logger::default_logger().fatal("Failed to find suitable video device");
        }

        return std::make_pair(best_dev, best_dev_indices);
    }

    LogicalDevice create_vk_device(VkInstance instance) {
        VkPhysicalDevice phys_dev;
        QueueFamilyIndices qf_indices;
        std::tie(phys_dev, qf_indices) = _select_physical_device(instance);
        VkPhysicalDeviceProperties phys_dev_props;
        vkGetPhysicalDeviceProperties(phys_dev, &phys_dev_props);

        Logger::default_logger().info("Selected video device %s", phys_dev_props.deviceName);

        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = qf_indices.graphics_family;
        queue_create_info.queueCount = 1;

        auto queue_priority = 1.0f;
        queue_create_info.pQueuePriorities = &queue_priority;

        VkPhysicalDeviceFeatures dev_features{};

        VkDeviceCreateInfo dev_create_info{};
        dev_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dev_create_info.pQueueCreateInfos = &queue_create_info;
        dev_create_info.queueCreateInfoCount = 1;
        dev_create_info.pEnabledFeatures = &dev_features;
        dev_create_info.enabledExtensionCount = 0;

        uint32_t layers_count;
        const char *const *layers;

        #ifdef _ARGUS_DEBUG_MODE
        layers_count = uint32_t(g_validation_layers.size());
        layers = g_validation_layers.data();
        #else
        layers_count = 0;
        layers = nullptr;
        #endif

        dev_create_info.enabledLayerCount = layers_count;
        dev_create_info.ppEnabledLayerNames = layers;

        VkDevice dev;
        auto rc = vkCreateDevice(phys_dev, &dev_create_info, nullptr, &dev);
        if (rc != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create logical Vulkan device (rc: %d)", rc);
        }

        VkQueue graphics_queue;
        vkGetDeviceQueue(dev, qf_indices.graphics_family, 0, &graphics_queue);

        Logger::default_logger().debug("Successfully created logical Vulkan device");

        return { dev, graphics_queue };
    }

    void destroy_vk_device(LogicalDevice device) {
        vkDestroyDevice(device.device, nullptr);
    }
}

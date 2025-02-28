/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#pragma once

#include "internal/render_vulkan/setup/queues.hpp"

#include "vulkan/vulkan.h"

#include <optional>

namespace argus {
    struct LogicalDevice {
        VkPhysicalDevice physical_device;
        VkDevice logical_device;
        QueueFamilyIndices queue_indices;
        QueueFamilies queues;
        QueueMutexes *queue_mutexes;
    };

    std::optional<LogicalDevice> create_vk_device(VkInstance instance, VkSurfaceKHR probe_surface);

    void destroy_vk_device(LogicalDevice device);
}

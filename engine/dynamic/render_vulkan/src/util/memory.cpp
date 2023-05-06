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

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/util/memory.hpp"

namespace argus {
    uint32_t find_memory_type(const LogicalDevice &device, uint32_t type_filter, GraphicsMemoryPropCombos props) {
        VkPhysicalDeviceMemoryProperties mem_props;
        vkGetPhysicalDeviceMemoryProperties(device.physical_device, &mem_props);

        auto search_props = props;

        while (true) {
            for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
                if ((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & props) == props) {
                    return i;
                }
            }

            if (search_props == GraphicsMemoryPropCombos::DeviceRw) {
                search_props = GraphicsMemoryPropCombos::HostRw;
                continue;
            }

            break;
        }

        Logger::default_logger().fatal("Failed to find suitable memory type");
    }
}

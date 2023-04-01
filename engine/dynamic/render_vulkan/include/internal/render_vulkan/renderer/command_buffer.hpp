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

#pragma once

#include "vulkan/vulkan.h"

namespace argus {
    // forward declarations
    struct RendererState;

    VkCommandPool create_command_pool(const LogicalDevice &device);

    void destroy_command_pool(const LogicalDevice &device, VkCommandPool command_pool);

    std::vector<VkCommandBuffer> alloc_command_buffers(const RendererState &state, uint32_t count);

    void free_command_buffers(const RendererState &state, std::vector<VkCommandBuffer> buffers);
}

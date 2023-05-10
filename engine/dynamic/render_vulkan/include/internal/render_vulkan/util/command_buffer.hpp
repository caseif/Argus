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

#include <vector>

#include <cstdint>

namespace argus {
    // forward declarations
    struct LogicalDevice;
    struct RendererState;

    struct CommandBufferInfo {
        VkCommandBuffer handle;
        VkCommandPool pool;
    };

    VkCommandPool create_command_pool(const LogicalDevice &device, uint32_t queue_index);

    void destroy_command_pool(const LogicalDevice &device, VkCommandPool command_pool);

    std::vector<CommandBufferInfo> alloc_command_buffers(const LogicalDevice &device, VkCommandPool pool, uint32_t count);

    void free_command_buffers(const LogicalDevice &device, const std::vector<CommandBufferInfo> &buffers);

    void free_command_buffer(const LogicalDevice &device, const CommandBufferInfo &buffer);

    void begin_oneshot_commands(const LogicalDevice &device, const CommandBufferInfo &buffer);

    void end_command_buffer(const LogicalDevice &device, const CommandBufferInfo &buffer);

    void submit_command_buffer(const LogicalDevice &device, const CommandBufferInfo &buffer, VkQueue queue,
            VkFence fence, const std::vector<VkSemaphore> &wait_semaphores,
            std::vector<VkPipelineStageFlags> wait_stages, const std::vector<VkSemaphore> &signal_semaphores);
}

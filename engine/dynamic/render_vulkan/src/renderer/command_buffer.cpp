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

#include "internal/render_vulkan/module_render_vulkan.hpp"
#include "internal/render_vulkan/renderer/command_buffer.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"

#include "vulkan/vulkan.h"

namespace argus {
    VkCommandPool create_command_pool(const LogicalDevice &device) {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = g_vk_device.queue_indices.graphics_family;

        VkCommandPool command_pool{};
        if (vkCreateCommandPool(device.logical_device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create command pool");
        }

        return command_pool;
    }

    void destroy_command_pool(const LogicalDevice &device, VkCommandPool command_pool) {
        vkDestroyCommandPool(device.logical_device, command_pool, nullptr);
    }

    std::vector<VkCommandBuffer> alloc_command_buffers(const RendererState &state, uint32_t count) {
        VkCommandBufferAllocateInfo cb_alloc_info{};
        cb_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cb_alloc_info.commandPool = state.command_pool;
        cb_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cb_alloc_info.commandBufferCount = count;

        std::vector<VkCommandBuffer> buffers;
        buffers.resize(count);
        if (vkAllocateCommandBuffers(state.device, &cb_alloc_info, &buffers.data()) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to allocate command buffers");
        }

        return buffers;
    }

    void free_command_buffers(const RendererState &state, std::vector<VkCommandBuffer> buffers) {
        vkFreeCommandBuffers(state.device, state.command_pool, static_cast<uint32_t>(buffers.size()), buffers.data());
    }
}

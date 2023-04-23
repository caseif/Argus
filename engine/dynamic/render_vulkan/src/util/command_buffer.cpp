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
#include "internal/render_vulkan/util/command_buffer.hpp"
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

    std::vector<CommandBufferInfo> alloc_command_buffers(const LogicalDevice &device, VkCommandPool pool,
            uint32_t count) {
        VkCommandBufferAllocateInfo cb_alloc_info{};
        cb_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cb_alloc_info.commandPool = pool;
        cb_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cb_alloc_info.commandBufferCount = count;

        std::vector<VkCommandBuffer> handles;
        handles.resize(count);
        if (vkAllocateCommandBuffers(device.logical_device, &cb_alloc_info, handles.data()) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to allocate command buffers");
        }

        std::vector<CommandBufferInfo> buffers;
        buffers.resize(count);
        std::transform(handles.cbegin(), handles.cend(), buffers.begin(), [&pool] (auto handle) {
            CommandBufferInfo cb_info{};
            cb_info.pool = pool;
            cb_info.handle = handle;
            return cb_info;
        });

        return buffers;
    }

    void free_command_buffers(const LogicalDevice &device, std::vector<CommandBufferInfo> buffers) {
        if (buffers.size() == 0) {
            return;
        }

        std::vector<VkCommandBuffer> handles;
        handles.reserve(buffers.size());
        std::transform(buffers.cbegin(), buffers.cend(), handles.begin(), [] (const auto &buf) { return buf.handle; });

        vkFreeCommandBuffers(device.logical_device, buffers.front().pool,
                static_cast<uint32_t>(handles.size()), handles.data());
    }

    void free_command_buffer(const LogicalDevice &device, CommandBufferInfo buffer) {
        vkFreeCommandBuffers(device.logical_device, buffer.pool, 1, &buffer.handle);
    }

    void begin_oneshot_commands(const LogicalDevice &device, const CommandBufferInfo &buffer) {
        UNUSED(device);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(buffer.handle, &begin_info);
    }

    void end_oneshot_commands(const LogicalDevice &device, const CommandBufferInfo &buffer) {
        vkEndCommandBuffer(buffer.handle);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &buffer.handle;
        vkQueueSubmit(device.queues.graphics_family, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(device.queues.graphics_family);

        vkResetCommandBuffer(buffer.handle, 0);
    }
}

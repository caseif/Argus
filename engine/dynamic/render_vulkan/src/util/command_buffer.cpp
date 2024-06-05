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

#include "argus/lowlevel/logging.hpp"

#include "internal/render_vulkan/module_render_vulkan.hpp"
#include "internal/render_vulkan/util/command_buffer.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"

#include "vulkan/vulkan.h"

#include <algorithm>

namespace argus {
    VkCommandPool create_command_pool(const LogicalDevice &device, uint32_t queue_index) {
        VkCommandPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = queue_index;

        VkCommandPool command_pool {};
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
        VkCommandBufferAllocateInfo cb_alloc_info {};
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
        std::transform(handles.cbegin(), handles.cend(), buffers.begin(), [&pool](auto handle) {
            CommandBufferInfo cb_info {};
            cb_info.pool = pool;
            cb_info.handle = handle;
            return cb_info;
        });

        return buffers;
    }

    void free_command_buffers(const LogicalDevice &device, const std::vector<CommandBufferInfo> &buffers) {
        if (buffers.empty()) {
            return;
        }

        std::vector<VkCommandBuffer> handles;
        handles.reserve(buffers.size());
        std::transform(buffers.cbegin(), buffers.cend(), handles.begin(), [](const auto &buf) { return buf.handle; });

        vkFreeCommandBuffers(device.logical_device, buffers.front().pool,
                static_cast<uint32_t>(handles.size()), handles.data());
    }

    void free_command_buffer(const LogicalDevice &device, const CommandBufferInfo &buffer) {
        vkFreeCommandBuffers(device.logical_device, buffer.pool, 1, &buffer.handle);
    }

    void begin_oneshot_commands(const LogicalDevice &device, const CommandBufferInfo &buffer) {
        UNUSED(device);

        vkResetCommandBuffer(buffer.handle, 0);

        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(buffer.handle, &begin_info);
    }

    void end_command_buffer(const LogicalDevice &device, const CommandBufferInfo &buffer) {
        UNUSED(device);

        vkEndCommandBuffer(buffer.handle);
    }

    void submit_command_buffer(const LogicalDevice &device, const CommandBufferInfo &buffer, VkQueue queue,
            VkFence fence, const std::vector<VkSemaphore> &wait_semaphores,
            const std::vector<VkPipelineStageFlags> &wait_stages,
            const std::vector<VkSemaphore> &signal_semaphores) {
        UNUSED(device);

        assert(wait_semaphores.size() == wait_stages.size());

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &buffer.handle;
        submit_info.waitSemaphoreCount = uint32_t(wait_semaphores.size());
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.signalSemaphoreCount = uint32_t(signal_semaphores.size());
        submit_info.pSignalSemaphores = signal_semaphores.data();
        if (vkQueueSubmit(queue, 1, &submit_info, fence) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to submit command queues");
        }
    }

    void queue_command_buffer_submit(RendererState &state, const CommandBufferInfo &buffer, VkQueue queue,
            VkFence fence, std::vector<VkSemaphore> wait_semaphores, std::vector<VkPipelineStageFlags> wait_stages,
            std::vector<VkSemaphore> signal_semaphores, Semaphore *submit_semaphore) {
        {
            std::lock_guard<std::mutex> lock(state.submit_mutex);
            state.submit_bufs.push_back(CommandBufferSubmitParams { false, 0, state.cur_frame, &buffer, queue, fence,
                    std::move(wait_semaphores), std::move(wait_stages), std::move(signal_semaphores),
                    submit_semaphore });
        }
        state.queued_submit_sem.notify();
    }
}

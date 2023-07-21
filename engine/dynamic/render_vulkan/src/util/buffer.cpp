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

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/util/memory.hpp"

#include "vulkan/vulkan.h"
#include "internal/render_vulkan/util/command_buffer.hpp"

namespace argus {

    BufferInfo alloc_buffer(const LogicalDevice &device, VkDeviceSize size, VkBufferUsageFlags usage,
            GraphicsMemoryPropCombos props) {
        assert(size > 0);

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer{};
        if (vkCreateBuffer(device.logical_device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create buffer");
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(device.logical_device, buffer, &mem_reqs);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = find_memory_type(device, mem_reqs.memoryTypeBits, props);

        VkDeviceMemory buffer_mem;
        if (vkAllocateMemory(device.logical_device, &alloc_info, nullptr, &buffer_mem) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device.logical_device, buffer, buffer_mem, 0);

        BufferInfo buf = { device.logical_device, buffer, buffer_mem, size, nullptr };
        if ((static_cast<VkMemoryPropertyFlagBits>(props) & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
            // map buffer persistently
            map_buffer(buf, 0, buf.size, 0);
        }

        return buf;
    }

    void free_buffer(BufferInfo &buffer) {
        assert(buffer.device != VK_NULL_HANDLE);
        assert(buffer.handle != VK_NULL_HANDLE);

        if (buffer.mapped != nullptr) {
            unmap_buffer(buffer);
        }

        vkFreeMemory(buffer.device, buffer.mem, nullptr);
        vkDestroyBuffer(buffer.device, buffer.handle, nullptr);

        buffer.device = {};
    }

    void *map_buffer(BufferInfo &buffer, VkDeviceSize offset, VkDeviceSize size,
            VkMemoryMapFlags flags) {
        assert(buffer.device != VK_NULL_HANDLE);
        assert(buffer.handle != VK_NULL_HANDLE);
        assert(size <= buffer.size);
        assert(buffer.mapped == nullptr);

        void *ptr;
        if (vkMapMemory(buffer.device, buffer.mem, offset, size, flags, &ptr) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to map buffer");
        }

        buffer.mapped = ptr;

        return ptr;
    }

    void unmap_buffer(BufferInfo &buffer) {
        assert(buffer.device != VK_NULL_HANDLE);
        assert(buffer.handle != VK_NULL_HANDLE);
        assert(buffer.mapped != nullptr);

        vkUnmapMemory(buffer.device, buffer.mem);
        buffer.mapped = nullptr;
    }

    void copy_buffer(const CommandBufferInfo &cmd_buf, const BufferInfo &src_buf,
            VkDeviceSize src_off, const BufferInfo &dst_buf, VkDeviceSize dst_off, size_t size) {
        assert(src_buf.device != VK_NULL_HANDLE);
        assert(src_buf.handle != VK_NULL_HANDLE);
        assert(dst_buf.device != VK_NULL_HANDLE);
        assert(dst_buf.handle != VK_NULL_HANDLE);

        VkBufferCopy copy_region{};
        copy_region.srcOffset = src_off;
        copy_region.dstOffset = dst_off;
        copy_region.size = size;
        vkCmdCopyBuffer(cmd_buf.handle, src_buf.handle, dst_buf.handle, 1, &copy_region);
    }
}

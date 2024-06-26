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

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/util/memory.hpp"

#include "vulkan/vulkan.h"
#include "command_buffer.hpp"

namespace argus {
    struct BufferInfo {
        VkDevice device;
        VkBuffer handle;
        VkDeviceMemory mem;
        VkDeviceSize size;
        void *mapped;
    };

    BufferInfo alloc_buffer(const LogicalDevice &device, VkDeviceSize size, VkBufferUsageFlags usage,
            GraphicsMemoryPropCombos props);

    void free_buffer(BufferInfo &buffer);

    void *map_buffer(BufferInfo &buffer, VkDeviceSize offset, VkDeviceSize size,
            VkMemoryMapFlags flags);

    void unmap_buffer(BufferInfo &buffer);

    void copy_buffer(const CommandBufferInfo &cmd_buf, const BufferInfo &src_buf,
            VkDeviceSize src_off, const BufferInfo &dst_buf, VkDeviceSize dst_off, size_t size);

    void write_to_buffer(BufferInfo &buffer, void *src, size_t offset, size_t len);

    template<typename T>
    void write_val_to_buffer(BufferInfo &buffer, T val, size_t offset) {
        write_to_buffer(buffer, reinterpret_cast<void *>(&val), offset, sizeof(val));
    }
}

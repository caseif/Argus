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
#include "argus/lowlevel/math.hpp"

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/util/image.hpp"
#include "internal/render_vulkan/util/memory.hpp"

#include "vulkan/vulkan.h"
#include "argus/render/common/texture_data.hpp"
#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/util/command_buffer.hpp"

#include <vector>

#include <cstdint>

namespace argus {
    VkImage create_image(const LogicalDevice &device, VkFormat format, const Vector2u &size,
            VkImageUsageFlags usage) {
        VkExtent3D extent = { size.x, size.y, 1 };
        std::vector<uint32_t> qf_indices{ device.queue_indices.graphics_family };

        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = format;
        image_info.extent = extent;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = usage;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.queueFamilyIndexCount = static_cast<uint32_t>(qf_indices.size());
        image_info.pQueueFamilyIndices = qf_indices.data();
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImage image;
        vkCreateImage(device.logical_device, &image_info, nullptr, &image);

        VkMemoryRequirements mem_reqs;
        vkGetImageMemoryRequirements(device.logical_device, image, &mem_reqs);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = mem_reqs.size;
        allocInfo.memoryTypeIndex = find_memory_type(device, mem_reqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkDeviceMemory imageMemory;
        if (vkAllocateMemory(device.logical_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to allocate memory for image");
        }

        if (vkBindImageMemory(device.logical_device, image, imageMemory, 0) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to bind image memory");
        }

        return image;
    }

    VkImageView create_image_view(const LogicalDevice &device, VkImage image, VkFormat format,
            VkImageAspectFlags aspect_mask) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = aspect_mask;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VkImageView view;
        vkCreateImageView(device.logical_device, &view_info, nullptr, &view);

        return view;
    }

    ImageInfo create_image_and_image_view(const LogicalDevice &device, VkFormat format, const Vector2u &size,
            VkImageUsageFlags usage, VkImageAspectFlags aspect_mask) {
        auto image = create_image(device, format, size, usage);
        auto view = create_image_view(device, image, format, aspect_mask);
        return ImageInfo { size, format, image, view };
    }

    void destroy_image(const LogicalDevice &device, VkImage image) {
        vkDestroyImage(device.logical_device, image, nullptr);
    }

    void destroy_image_view(const LogicalDevice &device, VkImageView view) {
        vkDestroyImageView(device.logical_device, view, nullptr);
    }

    void destroy_image_and_image_view(const LogicalDevice &device, const ImageInfo &image) {
        destroy_image_view(device, image.view);
        destroy_image(device, image.handle);
    }
}

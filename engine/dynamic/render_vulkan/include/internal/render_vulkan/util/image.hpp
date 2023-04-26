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

#include "argus/lowlevel/math.hpp"

#include "../setup/device.hpp"

#include "../../../../../../../external/libs/Vulkan-Headers/include/vulkan/vulkan.h"
#include "command_buffer.hpp"

#include <vector>

namespace argus {
    struct ImageInfo {
        Vector2u size;
        VkFormat format;
        VkImage handle;
        VkImageView view;
    };

    VkImage create_image(const LogicalDevice &device, VkFormat format, const Vector2u &size,
            VkImageUsageFlags usage);

    VkImageView create_image_view(const LogicalDevice &device, VkImage image, VkFormat format,
            VkImageAspectFlags aspect_mask);

    ImageInfo create_image_and_image_view(const LogicalDevice &device, VkFormat format, const Vector2u &size,
            VkImageUsageFlags usage, VkImageAspectFlags aspect_mask);

    void destroy_image(const LogicalDevice &device, VkImage image);

    void destroy_image_view(const LogicalDevice &device, VkImageView view);

    void destroy_image_and_image_view(const LogicalDevice &device, const ImageInfo &image);

    void perform_image_transition(const CommandBufferInfo &cmd_buf, VkImage image,
            VkImageLayout old_layout, VkImageLayout new_layout,
            VkAccessFlags src_access, VkAccessFlags dst_access,
            VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

    void perform_image_transition(const CommandBufferInfo &cmd_buf, const ImageInfo &image,
            VkImageLayout old_layout, VkImageLayout new_layout,
            VkAccessFlags src_access, VkAccessFlags dst_access,
            VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);
}

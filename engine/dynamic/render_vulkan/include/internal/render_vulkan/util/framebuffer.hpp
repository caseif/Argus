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

#include "internal/render_vulkan/util/image.hpp"
#include "internal/render_vulkan/util/pipeline.hpp"

#include "vulkan/vulkan.h"

namespace argus {
    struct FramebufferInfo {
        VkFramebuffer handle;
        ImageInfo image;
        VkSampler sampler;

        FramebufferInfo() :
            handle(VK_NULL_HANDLE),
            image({}),
            sampler(VK_NULL_HANDLE) {
        }
    };

    VkFramebuffer create_framebuffer(const LogicalDevice &device, VkRenderPass render_pass,
            const std::vector<VkImageView> &image_views, Vector2u size);

    VkFramebuffer create_framebuffer(const LogicalDevice &device, VkRenderPass render_pass,
            const std::vector<ImageInfo> &images);

    void destroy_framebuffer(const LogicalDevice &device, VkFramebuffer framebuffer);
}

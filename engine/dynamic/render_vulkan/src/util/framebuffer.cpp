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

#include "internal/render_vulkan/util/framebuffer.hpp"
#include "internal/render_vulkan/util/image.hpp"
#include "internal/render_vulkan/setup/device.hpp"

#include "vulkan/vulkan.h"

#include <vector>

namespace argus {
    VkFramebuffer create_framebuffer(const LogicalDevice &device, VkRenderPass render_pass,
            const std::vector<ImageInfo> &images) {
        assert(!images.empty());

        auto size = images.front().size;

        std::vector<VkImageView> image_views;
        image_views.reserve(images.size());
        for (const auto &image : images) {
            image_views.push_back(image.view);
        }

        VkFramebufferCreateInfo fb_info{};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = render_pass;
        fb_info.attachmentCount = static_cast<uint32_t>(image_views.size());
        fb_info.pAttachments = image_views.data();
        fb_info.width = size.x;
        fb_info.height = size.y;
        fb_info.layers = 1;

        VkFramebuffer fb;
        if (vkCreateFramebuffer(device.logical_device, &fb_info, nullptr, &fb) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create framebuffer");
        }

        return fb;
    }

    void destroy_framebuffer(const LogicalDevice &device, VkFramebuffer framebuffer) {
        vkDestroyFramebuffer(device.logical_device, framebuffer, nullptr);
    }
}

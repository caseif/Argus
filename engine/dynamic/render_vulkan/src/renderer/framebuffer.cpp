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

#include "internal/render_vulkan/renderer/framebuffer.hpp"

namespace argus {
    std::vector<VkFramebuffer> create_framebuffers(RendererState &state, PipelineInfo pipeline) {
        UNUSED(pipeline);

        std::vector<VkFramebuffer> framebuffers;
        framebuffers.reserve(state.swapchain_image_views.size());
        for (const auto &image_view : state.swapchain_image_views) {
            std::vector<VkImageView> image_views = { image_view };

            VkFramebufferCreateInfo fb_info;
            fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fb_info.renderPass = state.render_pass;
            fb_info.attachmentCount = static_cast<uint32_t>(image_views.size());
            fb_info.pAttachments = image_views.data();
            fb_info.width = state.viewport_size.x;
            fb_info.height = state.viewport_size.y;
            fb_info.layers = 1;

            VkFramebuffer fb;
            if (vkCreateFramebuffer(state.device, &fb_info, nullptr, &fb) != VK_SUCCESS) {
                Logger::default_logger().fatal("Failed to create framebuffer");
            }

            framebuffers.push_back(fb);
        }

        return framebuffers;
    }
}

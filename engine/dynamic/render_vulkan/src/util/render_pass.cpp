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

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/util/render_pass.hpp"

#include "vulkan/vulkan.h"

namespace argus {


    VkRenderPass create_render_pass(const LogicalDevice &device, VkFormat format, VkImageLayout final_layout) {
        VkAttachmentDescription color_att{};
        color_att.format = format;
        color_att.samples = VK_SAMPLE_COUNT_1_BIT;
        color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_att.finalLayout = final_layout;

        VkAttachmentReference color_att_ref{};
        color_att_ref.attachment = SHADER_OUT_COLOR_LOC;
        color_att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_att_ref;

        VkSubpassDependency subpass_dep{};
        subpass_dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dep.dstSubpass = 0;
        subpass_dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dep.srcAccessMask = 0;
        subpass_dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dep.dstAccessMask = 0;

        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &color_att;
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &subpass_dep;

        VkRenderPass render_pass;
        if (vkCreateRenderPass(device.logical_device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create render pass");
        }

        return render_pass;
    }

    void destroy_render_pass(const LogicalDevice &device, VkRenderPass render_pass) {
        vkDestroyRenderPass(device.logical_device, render_pass, nullptr);
    }
}

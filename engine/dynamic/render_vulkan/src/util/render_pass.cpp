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

#include "argus/core/engine.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/util/render_pass.hpp"

#include "vulkan/vulkan.h"

namespace argus {
    VkRenderPass create_render_pass(const LogicalDevice &device, VkFormat format, VkImageLayout final_layout,
            bool with_supp_attachments) {
        std::vector<VkAttachmentDescription> atts;
        std::vector<VkAttachmentReference> att_refs;
        atts.reserve(2);
        att_refs.reserve(2);

        VkAttachmentDescription color_att {};
        color_att.format = format;
        color_att.samples = VK_SAMPLE_COUNT_1_BIT;
        color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_att.finalLayout = final_layout;
        atts.push_back(color_att);

        VkAttachmentReference color_att_ref {};
        color_att_ref.attachment = SHADER_OUT_COLOR_LOC;
        color_att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        att_refs.push_back(color_att_ref);

        if (with_supp_attachments) {
            VkAttachmentDescription light_opac_att {};
            light_opac_att.format = VK_FORMAT_R32_SFLOAT;
            light_opac_att.samples = VK_SAMPLE_COUNT_1_BIT;
            light_opac_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            light_opac_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            light_opac_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            light_opac_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            light_opac_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            light_opac_att.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            atts.push_back(light_opac_att);

            VkAttachmentReference light_opac_att_ref {};
            light_opac_att_ref.attachment = SHADER_OUT_LIGHT_OPACITY_LOC;
            light_opac_att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            att_refs.push_back(light_opac_att_ref);
        }

        VkSubpassDescription subpass {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = uint32_t(att_refs.size());
        subpass.pColorAttachments = att_refs.data();

        VkSubpassDependency subpass_dep {};
        subpass_dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpass_dep.dstSubpass = 0;
        subpass_dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dep.srcAccessMask = 0;
        subpass_dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpass_dep.dstAccessMask = 0;

        VkRenderPassCreateInfo render_pass_info {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = uint32_t(atts.size());
        render_pass_info.pAttachments = atts.data();
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &subpass_dep;

        VkRenderPass render_pass;
        if (vkCreateRenderPass(device.logical_device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
            crash("Failed to create render pass");
        }

        return render_pass;
    }

    void destroy_render_pass(const LogicalDevice &device, VkRenderPass render_pass) {
        vkDestroyRenderPass(device.logical_device, render_pass, nullptr);
    }
}

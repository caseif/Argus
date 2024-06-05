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

#include "argus/lowlevel/debug.hpp"

#include "argus/render/defines.hpp"
#include "argus/render/common/material.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/util/descriptor_set.hpp"
#include "internal/render_vulkan/util/pipeline.hpp"
#include "internal/render_vulkan/renderer/shader_mgmt.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"

#include "vulkan/vulkan.h"

#include <vector>

#include <cstdint>

namespace argus {
    static void _push_attr(std::vector<VkVertexInputAttributeDescription> &attr_descs, uint32_t binding,
            uint32_t location, VkFormat format, uint32_t components, uint32_t &offset) {
        VkVertexInputAttributeDescription attr_desc {};
        attr_desc.binding = binding;
        attr_desc.location = location;
        attr_desc.format = format;
        attr_desc.offset = offset;
        attr_descs.push_back(attr_desc);
        offset += static_cast<uint32_t>(components * sizeof(float));
    }

    PipelineInfo create_pipeline(RendererState &state, const Material &material, VkRenderPass render_pass) {
        return create_pipeline(state, material.get_shader_uids(), render_pass);
    }

    PipelineInfo create_pipeline(RendererState &state, const std::vector<std::string> &shader_uids,
            VkRenderPass render_pass) {
        auto prepared_shaders = prepare_shaders(state.device.logical_device, shader_uids);

        auto shader_refl = prepared_shaders.reflection;

        std::vector<VkDynamicState> dyn_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dyn_state_info {};
        dyn_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dyn_state_info.dynamicStateCount = static_cast<uint32_t>(dyn_states.size());
        dyn_state_info.pDynamicStates = dyn_states.data();

        std::vector<VkVertexInputAttributeDescription> attr_descs;

        uint32_t offset = 0;
        shader_refl.get_attr_loc_and_then(SHADER_ATTRIB_POSITION, [&attr_descs, &offset](auto loc) {
            _push_attr(attr_descs, BINDING_INDEX_VBO, loc, SHADER_ATTRIB_POSITION_FORMAT,
                    SHADER_ATTRIB_POSITION_LEN, offset);
        });
        shader_refl.get_attr_loc_and_then(SHADER_ATTRIB_NORMAL, [&attr_descs, &offset](auto loc) {
            _push_attr(attr_descs, BINDING_INDEX_VBO, loc, SHADER_ATTRIB_NORMAL_FORMAT,
                    SHADER_ATTRIB_NORMAL_LEN, offset);
        });
        shader_refl.get_attr_loc_and_then(SHADER_ATTRIB_COLOR, [&attr_descs, &offset](auto loc) {
            _push_attr(attr_descs, BINDING_INDEX_VBO, loc, SHADER_ATTRIB_COLOR_FORMAT,
                    SHADER_ATTRIB_COLOR_LEN, offset);
        });
        shader_refl.get_attr_loc_and_then(SHADER_ATTRIB_TEXCOORD, [&attr_descs, &offset](auto loc) {
            _push_attr(attr_descs, BINDING_INDEX_VBO, loc, SHADER_ATTRIB_TEXCOORD_FORMAT,
                    SHADER_ATTRIB_TEXCOORD_LEN, offset);
        });

        std::vector<VkVertexInputBindingDescription> binding_descs;
        VkVertexInputBindingDescription vbo_desc {};
        vbo_desc.binding = BINDING_INDEX_VBO;
        vbo_desc.stride = offset;
        vbo_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        binding_descs.push_back(vbo_desc);

        auto anim_frame_loc = shader_refl.get_attr_loc(SHADER_ATTRIB_ANIM_FRAME);
        if (anim_frame_loc.has_value()) {
            uint32_t af_offset = 0;
            _push_attr(attr_descs, BINDING_INDEX_ANIM_FRAME_BUF, anim_frame_loc.value(),
                    SHADER_ATTRIB_ANIM_FRAME_FORMAT, SHADER_ATTRIB_ANIM_FRAME_LEN, af_offset);

            VkVertexInputBindingDescription anim_buf_desc {};
            anim_buf_desc.binding = BINDING_INDEX_ANIM_FRAME_BUF;
            anim_buf_desc.stride = af_offset;
            anim_buf_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            binding_descs.push_back(anim_buf_desc);
        }

        VkPipelineVertexInputStateCreateInfo vert_in_state_info {};
        vert_in_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vert_in_state_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descs.size());
        vert_in_state_info.pVertexBindingDescriptions = binding_descs.data();
        vert_in_state_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_descs.size());
        vert_in_state_info.pVertexAttributeDescriptions = attr_descs.data();

        VkPipelineInputAssemblyStateCreateInfo in_assembly_info {};
        in_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        in_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        in_assembly_info.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = float(state.viewport_size.x);
        viewport.height = float(state.viewport_size.y);
        viewport.minDepth = 0;
        viewport.maxDepth = 1;

        VkRect2D scissor {};
        scissor.offset = { 0, 0 };
        scissor.extent = { state.viewport_size.x, state.viewport_size.y };

        VkPipelineViewportStateCreateInfo viewport_info {};
        viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_info.viewportCount = 1;
        viewport_info.pViewports = &viewport;
        viewport_info.scissorCount = 1;
        viewport_info.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo raster_info {};
        raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster_info.depthClampEnable = VK_FALSE;
        raster_info.rasterizerDiscardEnable = VK_FALSE;
        raster_info.polygonMode = VK_POLYGON_MODE_FILL;
        raster_info.lineWidth = 1;
        raster_info.cullMode = VK_CULL_MODE_NONE;
        raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster_info.depthBiasEnable = VK_FALSE;
        raster_info.depthBiasConstantFactor = 0;
        raster_info.depthBiasClamp = 0;
        raster_info.depthBiasSlopeFactor = 0;

        VkPipelineMultisampleStateCreateInfo multisample_info {};
        multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_info.sampleShadingEnable = VK_FALSE;
        multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_info.minSampleShading = 1.0f;
        multisample_info.pSampleMask = nullptr;
        multisample_info.alphaToCoverageEnable = VK_FALSE;
        multisample_info.alphaToOneEnable = VK_FALSE;

        std::vector<VkPipelineColorBlendAttachmentState> atts;
        atts.reserve(2);

        VkPipelineColorBlendAttachmentState color_blend_att {};
        color_blend_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_att.blendEnable = VK_TRUE;
        color_blend_att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_att.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_att.alphaBlendOp = VK_BLEND_OP_ADD;

        atts.push_back(color_blend_att);

        if (shader_refl.get_output_loc(SHADER_OUT_LIGHT_OPACITY).has_value()) {
            VkPipelineColorBlendAttachmentState light_opac_blend_att {};
            light_opac_blend_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
            light_opac_blend_att.blendEnable = VK_TRUE;
            light_opac_blend_att.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            light_opac_blend_att.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            light_opac_blend_att.colorBlendOp = VK_BLEND_OP_ADD;
            light_opac_blend_att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            light_opac_blend_att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            light_opac_blend_att.alphaBlendOp = VK_BLEND_OP_ADD;
            atts.push_back(light_opac_blend_att);
        }

        VkPipelineColorBlendStateCreateInfo color_blend_info {};
        color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_info.logicOpEnable = VK_FALSE;
        color_blend_info.logicOp = VK_LOGIC_OP_COPY;
        color_blend_info.attachmentCount = uint32_t(atts.size());
        color_blend_info.pAttachments = atts.data();
        color_blend_info.blendConstants[0] = 0.0f;
        color_blend_info.blendConstants[1] = 0.0f;
        color_blend_info.blendConstants[2] = 0.0f;
        color_blend_info.blendConstants[3] = 0.0f;

        auto ds_layout = create_descriptor_set_layout(state.device, shader_refl);

        VkPipelineLayoutCreateInfo pipeline_layout_info {};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &ds_layout;
        pipeline_layout_info.pushConstantRangeCount = 0;
        pipeline_layout_info.pPushConstantRanges = nullptr;

        VkPipelineLayout pipeline_layout;
        vkCreatePipelineLayout(state.device.logical_device, &pipeline_layout_info, nullptr, &pipeline_layout);

        auto out_color_loc = shader_refl.get_output_loc(SHADER_OUT_COLOR);
        affirm_precond(out_color_loc.has_value(), "Required shader output " SHADER_OUT_COLOR " is missing");
        affirm_precond(out_color_loc.value() == SHADER_OUT_COLOR_LOC,
                "Required shader output " SHADER_OUT_COLOR " must have location 0");

        VkPipelineDepthStencilStateCreateInfo depth_info {};
        depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_info.depthTestEnable = VK_TRUE;
        depth_info.depthWriteEnable = VK_FALSE;
        depth_info.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depth_info.depthBoundsTestEnable = VK_FALSE;
        depth_info.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipeline_info {};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = static_cast<uint32_t>(prepared_shaders.stages.size());
        pipeline_info.pStages = prepared_shaders.stages.data();
        pipeline_info.pVertexInputState = &vert_in_state_info;
        pipeline_info.pInputAssemblyState = &in_assembly_info;
        pipeline_info.pViewportState = &viewport_info;
        pipeline_info.pRasterizationState = &raster_info;
        pipeline_info.pMultisampleState = &multisample_info;
        pipeline_info.pDepthStencilState = nullptr;
        pipeline_info.pColorBlendState = &color_blend_info;
        pipeline_info.pDynamicState = &dyn_state_info;
        pipeline_info.pDepthStencilState = &depth_info;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = render_pass;

        VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(state.device.logical_device, VK_NULL_HANDLE, 1,
                &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
            crash("Failed to create graphics pipeline");
        }

        destroy_shaders(state.device, prepared_shaders);

        PipelineInfo ret {};
        ret.handle = pipeline;
        ret.layout = pipeline_layout;
        ret.ds_layout = ds_layout;
        ret.reflection = shader_refl;
        ret.vertex_len = offset;

        return ret;
    }

    void destroy_pipeline(const LogicalDevice &device, const PipelineInfo &pipeline) {
        destroy_descriptor_set_layout(device, pipeline.ds_layout);

        vkDestroyPipeline(device.logical_device, pipeline.handle, nullptr);
        vkDestroyPipelineLayout(device.logical_device, pipeline.layout, nullptr);
    }
}

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

#include "argus/render/common/material.hpp"
#include "argus/render/common/shader.hpp"

#include "../setup/device.hpp"

#include "../../../../../../../external/libs/Vulkan-Headers/include/vulkan/vulkan.h"

#include <string>
#include <vector>

namespace argus {
    // forward declarations
    struct RendererState;

    struct PipelineInfo {
        VkPipeline handle;
        VkPipelineLayout layout;
        ShaderReflectionInfo reflection;
        uint32_t vertex_len;

        VkDescriptorSetLayout ds_layout;
        std::vector<VkDescriptorSet> desc_sets;
    };

    VkRenderPass create_render_pass(const LogicalDevice &device, VkFormat format, VkImageLayout final_layout);

    PipelineInfo create_pipeline(RendererState &state, const Material &material, VkRenderPass render_pass);

    PipelineInfo create_pipeline(RendererState &state, const std::vector<std::string> &shader_uids,
            VkRenderPass render_pass);

    void destroy_pipeline(const PipelineInfo &pipeline);
}

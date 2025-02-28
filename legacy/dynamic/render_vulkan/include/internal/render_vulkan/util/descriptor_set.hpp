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

#include "argus/render/common/shader.hpp"

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/util/buffer.hpp"

#include "vulkan/vulkan.h"

#include <vector>

namespace argus {
    struct DescriptorSetInfo {
        VkDescriptorSet handle;
        BufferInfo buffer;
    };

    VkDescriptorPool create_descriptor_pool(const LogicalDevice &device);

    void destroy_descriptor_pool(const LogicalDevice &device, VkDescriptorPool pool);

    VkDescriptorSetLayout create_descriptor_set_layout(const LogicalDevice &device,
            const ShaderReflectionInfo &shader_refl);

    void destroy_descriptor_set_layout(const LogicalDevice &device, VkDescriptorSetLayout layout);

    std::vector<VkDescriptorSet> create_descriptor_sets(const LogicalDevice &device, VkDescriptorPool pool,
            const ShaderReflectionInfo &shader_refl);

    void destroy_descriptor_sets(const LogicalDevice &device, VkDescriptorPool pool,
            const std::vector<VkDescriptorSet> &sets);
}

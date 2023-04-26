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

#include "argus/lowlevel/debug.hpp"

#include "argus/render/common/shader.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/util/descriptor_set.hpp"

#include "vulkan/vulkan.h"

namespace argus {
    static constexpr const uint32_t INITIAL_VIEWPORT_CAP = 2;
    static constexpr const uint32_t INITIAL_BUCKET_CAP = 64;
    static constexpr const uint32_t SAMPLERS_PER_BUCKET = 1;
    static constexpr const uint32_t UBOS_PER_BUCKET = 3;
    static constexpr const uint32_t INITIAL_DS_COUNT = INITIAL_VIEWPORT_CAP * INITIAL_BUCKET_CAP * MAX_FRAMES_IN_FLIGHT;
    static constexpr const uint32_t INITIAL_UBO_COUNT = INITIAL_DS_COUNT * UBOS_PER_BUCKET;
    static constexpr const uint32_t INITIAL_SAMPLER_COUNT = INITIAL_DS_COUNT * SAMPLERS_PER_BUCKET;

    static std::vector<VkDescriptorSetLayoutBinding> _create_ubo_bindings(const ShaderReflectionInfo &shader_refl) {
        auto count = shader_refl.ubo_bindings.size();
        affirm_precond(count <= UINT32_MAX, "Too many UBOs");

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(count);

        for (const auto &ubo : shader_refl.ubo_bindings) {
            VkDescriptorSetLayoutBinding ubo_binding{};
            ubo_binding.binding = ubo.second;
            ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ubo_binding.descriptorCount = 1; //TODO: account for array UBOs
            ubo_binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
            ubo_binding.pImmutableSamplers = nullptr;

            bindings.push_back(ubo_binding);
        }

        return bindings;
    }

    static VkDescriptorSetLayoutBinding _create_sampler_binding(void) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0; //TODO: pass actual value through reflection info
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        binding.pImmutableSamplers = nullptr;

        return binding;
    }

    static VkDescriptorSetLayout _create_descriptor_set_layout(const LogicalDevice &device,
            const std::vector<VkDescriptorSetLayoutBinding> &bindings) {
        affirm_precond(bindings.size() <= UINT32_MAX, "Too many descriptor set layout bindings");

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = uint32_t(bindings.size());
        layout_info.pBindings = bindings.data();

        VkDescriptorSetLayout layout;
        if (vkCreateDescriptorSetLayout(device.logical_device, &layout_info, nullptr, &layout) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create UBO descriptor set layout");
        }

        return layout;
    }

    VkDescriptorPool create_descriptor_pool(const LogicalDevice &device) {
        VkDescriptorPoolSize ubo_pool_size{};
        ubo_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_pool_size.descriptorCount = INITIAL_UBO_COUNT;

        VkDescriptorPoolSize sampler_pool_size{};
        sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_pool_size.descriptorCount = INITIAL_SAMPLER_COUNT;

        VkDescriptorPoolSize pool_sizes[] = { ubo_pool_size, sampler_pool_size };

        VkDescriptorPoolCreateInfo desc_pool_info{};
        desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        desc_pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
        desc_pool_info.pPoolSizes = pool_sizes;
        desc_pool_info.maxSets = INITIAL_DS_COUNT;
        desc_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkDescriptorPool pool;
        if (vkCreateDescriptorPool(device.logical_device, &desc_pool_info, nullptr, &pool) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create descriptor pool");
        }

        return pool;
    }

    void destroy_descriptor_pool(const LogicalDevice &device, VkDescriptorPool pool) {
        vkDestroyDescriptorPool(device.logical_device, pool, nullptr);
    }

    VkDescriptorSetLayout create_descriptor_set_layout(const LogicalDevice &device,
            const ShaderReflectionInfo &shader_refl) {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        auto ubo_bindings = _create_ubo_bindings(shader_refl);
        auto sampler_binding = _create_sampler_binding();
        bindings.insert(bindings.begin(), ubo_bindings.cbegin(), ubo_bindings.cend());
        bindings.push_back(sampler_binding);

        return _create_descriptor_set_layout(device, bindings);
    }

    void destroy_descriptor_set_layout(const LogicalDevice &device, VkDescriptorSetLayout layout) {
        vkDestroyDescriptorSetLayout(device.logical_device, layout, nullptr);
    }

    std::vector<VkDescriptorSet> create_descriptor_sets(const LogicalDevice &device, VkDescriptorPool pool,
            const ShaderReflectionInfo &shader_refl) {
        //auto count = uint32_t(MAX_FRAMES_IN_FLIGHT);
        uint32_t count = 1;
        std::vector<VkDescriptorSetLayout> layouts(count, create_descriptor_set_layout(device, shader_refl));

        VkDescriptorSetAllocateInfo ds_info{};
        ds_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ds_info.descriptorPool = pool;
        ds_info.descriptorSetCount = count;
        ds_info.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> sets;
        sets.resize(count);
        if (vkAllocateDescriptorSets(device.logical_device, &ds_info, sets.data()) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to allocate descriptor sets");
        }

        return sets;
    }

    void destroy_descriptor_sets(const LogicalDevice &device, VkDescriptorPool pool,
            const std::vector<VkDescriptorSet> &sets) {
        vkFreeDescriptorSets(device.logical_device, pool, uint32_t(sets.size()), sets.data());
    }
}

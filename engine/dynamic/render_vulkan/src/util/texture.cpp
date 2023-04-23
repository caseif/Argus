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

#include "argus/resman/resource.hpp"

#include "argus/render/common/material.hpp"

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/util/command_buffer.hpp"
#include "internal/render_vulkan/util/image.hpp"
#include "internal/render_vulkan/util/texture.hpp"

namespace argus {
    static void _apply_image_barrier(const CommandBufferInfo &cmd_buf, const ImageInfo &image,
            VkImageLayout old_layout, VkImageLayout new_layout) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags src_stage;
        VkPipelineStageFlags dst_stage;
        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            Logger::default_logger().fatal("Unsupported image layout transition");
        }

        vkCmdPipelineBarrier(cmd_buf.handle, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    PreparedTexture prepare_texture(const LogicalDevice &device, VkCommandPool pool, const TextureData &texture) {
        uint32_t channels = 4;
        VkDeviceSize image_size = texture.width * texture.height * channels;

        auto format = VK_FORMAT_R8G8B8A8_SRGB;

        auto image = create_image_and_image_view(device, format, { texture.width, texture.height },
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        auto staging_buf = alloc_buffer(device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        {
            auto buf_mapped = map_buffer(device, staging_buf, 0, image_size, 0);
            memcpy(buf_mapped, texture.get_pixel_data(), image_size);
            unmap_buffer(device, staging_buf);
        }

        auto must_free_pool = false;
        if (pool == VK_NULL_HANDLE) {
            pool = create_command_pool(device);
            must_free_pool = true;
        }

        auto cmd_buf = alloc_command_buffers(device, pool, 1).front();
        begin_oneshot_commands(device, cmd_buf);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = { texture.width, texture.height, 1 };

        _apply_image_barrier(cmd_buf, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdCopyBufferToImage(cmd_buf.handle, staging_buf.handle, image.handle,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        end_oneshot_commands(device, cmd_buf);

        free_command_buffer(device, cmd_buf);

        if (must_free_pool) {
            destroy_command_pool(device, pool);
        }

        free_buffer(device, staging_buf);

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_NEAREST;
        sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.anisotropyEnable = VK_FALSE;
        sampler_info.maxAnisotropy = 0;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;

        VkSampler sampler;
        vkCreateSampler(device.logical_device, &sampler_info, nullptr, &sampler);

        return { "", image, sampler };
    }

    void get_or_load_texture(RendererState &state, const Resource &material_res) {
        auto &texture_uid = material_res.get<Material>().get_texture_uid();

        auto existing_it = state.prepared_textures.find(texture_uid);
        if (existing_it != state.prepared_textures.end()) {
            existing_it->second.acquire();
            state.material_textures.insert({material_res.uid, texture_uid});
            return;
        }

        auto &texture_res = ResourceManager::instance().get_resource(texture_uid);
        auto &texture = texture_res.get<TextureData>();

        auto prepared = prepare_texture(state.device, VK_NULL_HANDLE, texture);

        texture_res.release();

        state.prepared_textures.insert({ texture_uid, prepared });
        state.material_textures.insert({ material_res.uid, texture_uid });
    }
}

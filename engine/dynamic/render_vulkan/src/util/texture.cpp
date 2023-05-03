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
    PreparedTexture prepare_texture(const LogicalDevice &device, const CommandBufferInfo &cmd_buf,
            const Resource &texture_res) {
        const auto &texture = texture_res.get<TextureData>();

        uint32_t channels = 4;
        VkDeviceSize image_size = texture.width * texture.height * channels;

        auto format = VK_FORMAT_R8G8B8A8_SRGB;

        auto image = create_image_and_image_view(device, format, { texture.width, texture.height },
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        auto staging_buf = alloc_buffer(device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        {
            auto buf_mapped = map_buffer(device, staging_buf, 0, image_size, 0);
            const size_t bytes_per_pixel = 4;
            const size_t bytes_per_row = texture.width * bytes_per_pixel;
            for (size_t y = 0; y < texture.height; y++) {
                auto dst = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buf_mapped) + (y * bytes_per_row));
                memcpy(dst, texture.get_pixel_data()[y], bytes_per_row);
            }
            unmap_buffer(device, staging_buf);
        }

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

        perform_image_transition(cmd_buf, image,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        vkCmdCopyBufferToImage(cmd_buf.handle, staging_buf.handle, image.handle,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        perform_image_transition(cmd_buf, image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

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

        return { texture_res.uid, image, sampler, staging_buf };
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

        auto prepared = prepare_texture(state.device, state.copy_cmd_buf, texture_res);

        texture_res.release();

        state.prepared_textures.insert({ texture_uid, prepared });
        state.material_textures.insert({ material_res.uid, texture_uid });

        state.texture_bufs_to_free.push_back(prepared.staging_buf);
    }

    void destroy_texture(const LogicalDevice &device, const PreparedTexture &texture) {
        vkDestroySampler(device.logical_device, texture.sampler, nullptr);
        destroy_image_and_image_view(device, texture.image);
    }
}

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

#include "argus/resman/resource.hpp"

#include "argus/render/common/texture_data.hpp"

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/util/image.hpp"

#include "vulkan/vulkan.h"

#include <string>

namespace argus {
    struct PreparedTexture {
        std::string uid;
        ImageInfo image;
        VkSampler sampler;
        BufferInfo staging_buf;
    };

    PreparedTexture prepare_texture(const LogicalDevice &device, const CommandBufferInfo &cmd_buf,
            const Resource &texture_res);

    void get_or_load_texture(RendererState &state, const Resource &material_res);

    void destroy_texture(const LogicalDevice &device, const PreparedTexture &texture);
}

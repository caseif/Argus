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

#include "argus/resman.hpp"

#include "internal/render_vulkan/renderer/pipeline.hpp"
#include "internal/render_vulkan/setup/swapchain.hpp"

#include "vulkan/vulkan.h"

#include <map>
#include <string>

namespace argus {
    struct RendererState {
        VkDevice device;

        Vector2u viewport_size;

        VkSurfaceKHR surface;
        SwapchainInfo swapchain;
        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        VkRenderPass render_pass;

        std::map<std::string, const Resource*> material_resources;
        std::map<std::string, PipelineInfo> material_pipelines;
    };
}

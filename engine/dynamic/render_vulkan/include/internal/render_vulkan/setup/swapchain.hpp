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

#include "argus/wm/window.hpp"

#include "internal/render_vulkan/setup/device.hpp"

#include "vulkan/vulkan.h"

#include <vector>

namespace argus {
    struct SwapchainSupportInfo {
        VkSurfaceCapabilitiesKHR caps;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    struct SwapchainInfo {
        VkSwapchainKHR swapchain;
        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
        VkFormat image_format;
        VkExtent2D extent;
    };

    SwapchainSupportInfo query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR probe_surface);

    SwapchainInfo create_vk_swapchain(const Window &window, LogicalDevice device, VkSurfaceKHR surface);
}

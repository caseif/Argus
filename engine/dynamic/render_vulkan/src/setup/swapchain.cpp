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

#include "argus/wm/window.hpp"

#include "internal/render_vulkan/setup/device.hpp"
#include "internal/render_vulkan/setup/swapchain.hpp"

#include <algorithm>

#include <cstdint>

namespace argus {
    SwapchainSupportInfo query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapchainSupportInfo support_info;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support_info.caps);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
        if (format_count > 0) {
            support_info.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, support_info.formats.data());
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
        if (present_mode_count > 0) {
            support_info.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count,
                    support_info.present_modes.data());
        }

        return support_info;
    }

    VkSurfaceFormatKHR _select_swap_surface_format(const SwapchainSupportInfo &support_info) {
        for (const auto &format : support_info.formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return support_info.formats.at(0);
    }

    VkPresentModeKHR _select_swap_present_mode(const SwapchainSupportInfo &support_info) {
        for (const auto &present_mode : support_info.present_modes) {
            if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return present_mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D _select_swap_extent(const VkSurfaceCapabilitiesKHR &caps, const Window &window) {
        if (caps.currentExtent.width != UINT32_MAX) {
            return caps.currentExtent;
        }

        return {
                std::clamp(window.peek_resolution().x, caps.minImageExtent.width, caps.maxImageExtent.width),
                std::clamp(window.peek_resolution().y, caps.minImageExtent.height, caps.maxImageExtent.height)
        };
    }

    SwapchainInfo create_vk_swapchain(const Window &window, const LogicalDevice &device, VkSurfaceKHR surface) {
        auto support_info = query_swapchain_support(device.physical_device, surface);
        affirm_precond(!support_info.formats.empty(), "No available swapchain formats");
        affirm_precond(!support_info.present_modes.empty(), "No available swapchain present modes");

        auto format = _select_swap_surface_format(support_info);
        auto present_mode = _select_swap_present_mode(support_info);
        auto extent = _select_swap_extent(support_info.caps, window);

        auto image_count = support_info.caps.minImageCount + 1;
        if (support_info.caps.maxImageCount != 0 && image_count > support_info.caps.maxImageCount) {
            image_count = support_info.caps.maxImageCount;
        }

        VkSwapchainCreateInfoKHR sc_create_info{};
        sc_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        sc_create_info.surface = surface;
        sc_create_info.minImageCount = image_count;
        sc_create_info.imageFormat = format.format;
        sc_create_info.imageColorSpace = format.colorSpace;
        sc_create_info.imageExtent = extent;
        sc_create_info.imageArrayLayers = 1;
        sc_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        uint32_t queue_indices[] = { device.queue_indices.graphics_family, device.queue_indices.present_family };

        if (device.queue_indices.graphics_family == device.queue_indices.present_family) {
            sc_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            sc_create_info.queueFamilyIndexCount = 0;
            sc_create_info.pQueueFamilyIndices = nullptr;
        } else {
            sc_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            sc_create_info.queueFamilyIndexCount = 2;
            sc_create_info.pQueueFamilyIndices = queue_indices;
        }

        sc_create_info.preTransform = support_info.caps.currentTransform;
        sc_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        sc_create_info.presentMode = present_mode;
        sc_create_info.clipped = VK_TRUE;
        sc_create_info.oldSwapchain = VK_NULL_HANDLE;

        SwapchainInfo sc_info;
        if (vkCreateSwapchainKHR(device.logical_device, &sc_create_info, nullptr, &sc_info.swapchain) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create Vulkan swapchain");
        }

        uint32_t real_image_count;

        vkGetSwapchainImagesKHR(device.logical_device, sc_info.swapchain, &real_image_count, nullptr);
        sc_info.images.resize(real_image_count);
        //sc_info.image_views.reserve(real_image_count);
        vkGetSwapchainImagesKHR(device.logical_device, sc_info.swapchain, &real_image_count, sc_info.images.data());

        /*for (auto sc_image : sc_info.images) {
            VkImageViewCreateInfo image_view_info{};
            image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_info.image = sc_image;
            image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_info.format = format.format;
            image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_info.subresourceRange.baseMipLevel = 0;
            image_view_info.subresourceRange.levelCount = 1;
            image_view_info.subresourceRange.baseArrayLayer = 0;
            image_view_info.subresourceRange.layerCount = 1;

            VkImageView image_view;
            if (vkCreateImageView(device.logical_device, &image_view_info, nullptr, &image_view) != VK_SUCCESS) {
                Logger::default_logger().fatal("Failed to create Vulkan image view");
            }

            sc_info.image_views.push_back(image_view);
        }*/

        sc_info.image_format = format.format;
        sc_info.extent = extent;

        return sc_info;
    }
}
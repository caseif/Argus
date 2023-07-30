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
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/util/framebuffer.hpp"
#include "internal/render_vulkan/util/image.hpp"
#include "internal/render_vulkan/util/render_pass.hpp"

#include <algorithm>
#include <mutex>

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

    static VkSurfaceFormatKHR _select_swap_surface_format(const SwapchainSupportInfo &support_info) {
        for (const auto &format : support_info.formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return support_info.formats.at(0);
    }

    static VkPresentModeKHR _select_swap_present_mode(const SwapchainSupportInfo &support_info) {
        for (const auto &present_mode : support_info.present_modes) {
            if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return present_mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static VkExtent2D _select_swap_extent(const VkSurfaceCapabilitiesKHR &caps, const Vector2u &resolution) {
        if (caps.currentExtent.width != UINT32_MAX) {
            return caps.currentExtent;
        }

        return {
                std::clamp(uint32_t(resolution.x), caps.minImageExtent.width, caps.maxImageExtent.width),
                std::clamp(uint32_t(resolution.y), caps.minImageExtent.height, caps.maxImageExtent.height)
        };
    }

    SwapchainInfo create_swapchain(const RendererState &state, VkSurfaceKHR surface, const Vector2u &resolution) {
        auto &device = state.device;
        auto support_info = query_swapchain_support(device.physical_device, surface);
        affirm_precond(!support_info.formats.empty(), "No available swapchain formats");
        affirm_precond(!support_info.present_modes.empty(), "No available swapchain present modes");

        auto format = _select_swap_surface_format(support_info);
        auto present_mode = _select_swap_present_mode(support_info);
        auto extent = _select_swap_extent(support_info.caps, resolution);

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
        sc_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
        if (vkCreateSwapchainKHR(device.logical_device, &sc_create_info, nullptr, &sc_info.handle) != VK_SUCCESS) {
            Logger::default_logger().fatal("Failed to create Vulkan swapchain");
        }

        sc_info.resolution = resolution;
        sc_info.surface = surface;
        sc_info.image_format = format.format;
        sc_info.extent = extent;

        // need to create the render pass before we create the framebuffers
        sc_info.composite_render_pass = create_render_pass(state.device, format.format,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        uint32_t real_image_count;

        vkGetSwapchainImagesKHR(device.logical_device, sc_info.handle, &real_image_count, nullptr);
        sc_info.images.resize(real_image_count);
        vkGetSwapchainImagesKHR(device.logical_device, sc_info.handle, &real_image_count, sc_info.images.data());

        for (auto sc_image : sc_info.images) {
            auto image_view = create_image_view(device, sc_image, format.format, VK_IMAGE_ASPECT_COLOR_BIT);
            sc_info.image_views.push_back(image_view);

            auto framebuffer = create_framebuffer(device, sc_info.composite_render_pass, { image_view },
                    { extent.width, extent.height });
            sc_info.framebuffers.push_back(framebuffer);
        }

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo sem_info{};
            sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            if (vkCreateSemaphore(device.logical_device, &sem_info, nullptr, &sc_info.image_avail_sem[i]) != VK_SUCCESS
                || vkCreateSemaphore(device.logical_device, &sem_info, nullptr, &sc_info.render_done_sem[i])
                   != VK_SUCCESS) {
                Logger::default_logger().fatal("Failed to create swapchain semaphores");
            }

            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            if (vkCreateFence(device.logical_device, &fence_info, nullptr, &sc_info.in_flight_fence[i]) != VK_SUCCESS) {
                Logger::default_logger().fatal("Failed to create swapchain fences");
            }
        }

        return sc_info;
    }

    void recreate_swapchain(const RendererState &state, const Vector2u &new_resolution, SwapchainInfo &swapchain) {
        std::lock_guard<std::mutex> lock(state.device.queue_mutexes->graphics_family);

        vkDeviceWaitIdle(state.device.logical_device);

        destroy_swapchain(state, swapchain);

        swapchain = create_swapchain(state, swapchain.surface, new_resolution);
    }

    void destroy_swapchain(const RendererState &state, const SwapchainInfo &swapchain) {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkWaitForFences(state.device.logical_device, 1, &swapchain.in_flight_fence[i], VK_TRUE, UINT64_MAX);

            vkDestroySemaphore(state.device.logical_device, swapchain.image_avail_sem[i], nullptr);
            vkDestroySemaphore(state.device.logical_device, swapchain.render_done_sem[i], nullptr);

            vkDestroyFence(state.device.logical_device, swapchain.in_flight_fence[i], nullptr);
        }

        for (const auto &fb : swapchain.framebuffers) {
            destroy_framebuffer(state.device, fb);
        }

        for (const auto &image_view : swapchain.image_views) {
            destroy_image_view(state.device, image_view);
        }

        destroy_render_pass(state.device, swapchain.composite_render_pass);

        vkDestroySwapchainKHR(state.device.logical_device, swapchain.handle, nullptr);
    }
}

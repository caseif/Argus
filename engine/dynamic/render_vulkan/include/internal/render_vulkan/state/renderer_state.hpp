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

#include "argus/lowlevel/refcountable.hpp"

#include "argus/resman.hpp"

#include "argus/render/common/attached_viewport.hpp"

#include "internal/render_vulkan/setup/swapchain.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"
#include "internal/render_vulkan/state/viewport_state.hpp"
#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/util/pipeline.hpp"
#include "internal/render_vulkan/util/texture.hpp"

#include "vulkan/vulkan.h"

#include <map>
#include <string>

namespace argus {
    struct RendererState {
        LogicalDevice device;

        Vector2u viewport_size;

        VkSurfaceKHR surface;
        SwapchainInfo swapchain;

        PipelineInfo composite_pipeline{};
        BufferInfo composite_vbo;

        VkCommandPool graphics_command_pool;
        VkDescriptorPool desc_pool;

        VkRenderPass fb_render_pass;

        CommandBufferInfo copy_cmd_buf{};
        std::map<uint32_t, std::pair<CommandBufferInfo, bool>> composite_cmd_bufs{};

        BufferInfo global_ubo{};

        std::map<const Scene2D *, Scene2DState> scene_states_2d;
        std::vector<SceneState *> all_scene_states;
        std::map<const AttachedViewport2D *, Viewport2DState> viewport_states_2d;

         bool dirty_viewports;

        std::map<std::string, const Resource*> material_resources;
        std::map<std::string, PipelineInfo> material_pipelines;
        std::map<std::string, RefCountable<PreparedTexture>> prepared_textures;
        std::map<std::string, std::string> material_textures;
        std::vector<BufferInfo> texture_bufs_to_free;

        VkSemaphore composite_semaphore;

        SceneState &get_scene_state(Scene &scene);

        ViewportState &get_viewport_state(AttachedViewport &viewport);
    };
}

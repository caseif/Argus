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

#include "argus/lowlevel/refcountable.hpp"
#include "argus/lowlevel/threading.hpp"

#include "argus/resman.hpp"

#include "argus/render/common/attached_viewport.hpp"

#include "internal/render_vulkan/defines.hpp"
#include "internal/render_vulkan/setup/swapchain.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"
#include "internal/render_vulkan/state/viewport_state.hpp"
#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/util/pipeline.hpp"
#include "internal/render_vulkan/util/texture.hpp"

#include "vulkan/vulkan.h"

#include <deque>
#include <map>
#include <mutex>
#include <queue>
#include <string>

namespace argus {
    struct CommandBufferSubmitParams {
        // ugly hack to synchronize present with command buffers
        bool is_present;
        uint32_t present_image_index;

        uint32_t cur_frame;
        const CommandBufferInfo *buffer;
        VkQueue queue;
        VkFence fence;
        std::vector<VkSemaphore> wait_sems;
        std::vector<VkPipelineStageFlags> wait_stages;
        std::vector<VkSemaphore> signal_sems;
        Semaphore *submit_sem;
    };

    struct RendererState {
        LogicalDevice device;

        Vector2u viewport_size;

        VkSurfaceKHR surface;
        SwapchainInfo swapchain;

        PipelineInfo composite_pipeline {};
        BufferInfo composite_vbo {};

        VkCommandPool graphics_command_pool;
        VkDescriptorPool desc_pool;

        VkRenderPass fb_render_pass;

        uint32_t cur_frame;

        CommandBufferInfo copy_cmd_buf[MAX_FRAMES_IN_FLIGHT];
        std::map<uint32_t, std::pair<CommandBufferInfo, bool>> composite_cmd_bufs;

        BufferInfo global_ubo {};

        std::map<const Scene2D *, Scene2DState> scene_states_2d;
        std::vector<SceneState *> all_scene_states;
        std::map<const AttachedViewport2D *, Viewport2DState> viewport_states_2d;
        bool are_viewports_initialized = false;

        bool dirty_viewports;

        std::map<std::string, const Resource *> material_resources;
        std::map<std::string, PipelineInfo> material_pipelines;
        std::map<std::string, RefCountable<PreparedTexture>> prepared_textures;
        std::map<std::string, std::string> material_textures;
        std::vector<BufferInfo> texture_bufs_to_free;

        VkSemaphore composite_semaphore;

        std::thread submit_thread;
        std::deque<CommandBufferSubmitParams> submit_bufs;
        std::mutex submit_mutex;
        Semaphore queued_submit_sem;
        bool submit_halt;
        Semaphore submit_halt_acked;

        Semaphore present_sem[MAX_FRAMES_IN_FLIGHT];
        Semaphore in_flight_sem[MAX_FRAMES_IN_FLIGHT];

        SceneState &get_scene_state(Scene &scene);

        ViewportState &get_viewport_state(AttachedViewport &viewport);
    };
}

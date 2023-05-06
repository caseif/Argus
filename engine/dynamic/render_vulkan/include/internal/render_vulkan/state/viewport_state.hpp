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

#include "argus/lowlevel/math.hpp"

#include "argus/render/2d/attached_viewport_2d.hpp"
#include "argus/render/common/attached_viewport.hpp"

#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/util/command_buffer.hpp"
#include "internal/render_vulkan/util/image.hpp"
#include "internal/render_vulkan/util/pipeline.hpp"

#include <map>

namespace argus {
    // forward declarations
    struct RendererState;

    struct ViewportState {
        RendererState &parent_state;
        AttachedViewport *viewport;

        bool visited;

        Matrix4 view_matrix;
        bool view_matrix_dirty;

        CommandBufferInfo command_buf;

        VkFence composite_fence;

        ImageInfo front_fb_image;
        ImageInfo back_fb_image;
        VkFramebuffer front_fb;
        VkFramebuffer back_fb;
        VkSampler front_fb_sampler;

        BufferInfo ubo;

        std::map<std::string, std::vector<VkDescriptorSet>> material_desc_sets;
        std::vector<VkDescriptorSet> composite_desc_sets;

        ViewportState(RendererState &parent_state, AttachedViewport *viewport);

        ViewportState(const ViewportState &rhs) = delete;

        ViewportState(ViewportState &&rhs) noexcept;
    };

    struct Viewport2DState : ViewportState {
        Viewport2DState(RendererState &parent_state, AttachedViewport2D *viewport);

        Viewport2DState(const Viewport2DState &rhs) = delete;

        Viewport2DState(Viewport2DState &&rhs) noexcept;
    };
}

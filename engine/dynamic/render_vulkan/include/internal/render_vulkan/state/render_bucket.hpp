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

#include "argus/render/common/material.hpp"

#include "internal/render_vulkan/util/buffer.hpp"
#include "internal/render_vulkan/state/processed_render_object.hpp"

namespace argus {
    struct RenderBucket {
      public:
        const Resource &material_res;
        const Vector2f atlas_stride;
        const uint32_t z_index;
        const float light_opacity;

        std::vector<ProcessedRenderObject *> objects;
        BufferInfo vertex_buffer;
        BufferInfo staging_vertex_buffer;
        BufferInfo anim_frame_buffer;
        BufferInfo staging_anim_frame_buffer;
        size_t vertex_count;

        BufferInfo ubo_buffer;

        bool needs_rebuild;

        static RenderBucket &create(const Resource &material_res, const Vector2f &atlas_stride, uint32_t z_index,
                float light_opacity);

        void destroy(void);

      private:
        RenderBucket(const Resource &material_res, const Vector2f &atlas_stride, uint32_t z_index,
                float light_opacity) :
            material_res(material_res),
            atlas_stride(atlas_stride),
            z_index(z_index),
            light_opacity(light_opacity),
            objects(),
            vertex_buffer(),
            staging_vertex_buffer(),
            anim_frame_buffer(),
            staging_anim_frame_buffer(),
            vertex_count(0),
            ubo_buffer({}),
            needs_rebuild(true) {
        }

        RenderBucket(RenderBucket &) = delete;

        ~RenderBucket();
    };
}

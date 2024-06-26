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

#include "argus/render/common/transform.hpp"

#include "internal/render_opengles/types.hpp"

#include <numeric>

#include <cstddef>
#include <cstring>

namespace argus {
    // forward definitions
    class Material;

    class RenderObject2D;

    struct ProcessedRenderObject {
        friend class PoolAllocator;

        const Resource &material_res;
        const Vector2f atlas_stride;
        const uint32_t z_index;
        const float light_opacity;

        Vector2u anim_frame;

        buffer_handle_t staging_buffer;
        size_t staging_buffer_size;
        size_t vertex_count;
        void *mapped_buffer;
        bool newly_created = false;
        bool visited = false;
        bool updated = false;
        bool anim_frame_updated = false;

        static ProcessedRenderObject &create(const Resource &material_res, const Vector2f &atlas_stride,
                uint32_t z_index, float light_opacity, buffer_handle_t staging_buffer, size_t staging_buffer_size,
                size_t vertex_count, void *mapped_buffer);

        ProcessedRenderObject(ProcessedRenderObject &) = delete;

        ~ProcessedRenderObject();

      private:
        ProcessedRenderObject(const Resource &material_res, const Vector2f &atlas_stride,
                uint32_t z_index, float light_opacity, buffer_handle_t staging_buffer, size_t staging_buffer_size,
                size_t vertex_count, void *mapped_buffer):
            material_res(material_res),
            atlas_stride(atlas_stride),
            z_index(z_index),
            light_opacity(light_opacity),
            staging_buffer(staging_buffer),
            staging_buffer_size(staging_buffer_size),
            vertex_count(vertex_count),
            mapped_buffer(mapped_buffer) {
        }
    };
}

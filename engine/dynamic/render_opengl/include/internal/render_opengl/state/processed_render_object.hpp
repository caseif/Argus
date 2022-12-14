/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "internal/render_opengl/types.hpp"

#include <numeric>

#include <cstddef>
#include <cstring>

namespace argus {
    // forward definitions
    class Material;
    class RenderObject2D;

    struct ProcessedRenderObject {
        friend class AllocPool;

        const Resource &material_res;
        const Vector2f atlas_stride;

        Vector2u anim_frame;

        buffer_handle_t staging_buffer;
        size_t staging_buffer_size;
        size_t vertex_count;
        void *mapped_buffer;
        bool newly_created;
        bool visited;
        bool updated;
        bool anim_frame_updated;

        static ProcessedRenderObject &create(const Resource &material_res, const Vector2f &atlas_stride,
                const buffer_handle_t staging_buffer, const size_t staging_buffer_size, const size_t vertex_count,
                void *mapped_buffer);

        ProcessedRenderObject(ProcessedRenderObject&) = delete;

        ~ProcessedRenderObject();

        private:
            ProcessedRenderObject(const Resource &material_res, const Vector2f &atlas_stride,
                    const buffer_handle_t staging_buffer, const size_t staging_buffer_size, const size_t vertex_count,
                    void *mapped_buffer):
                material_res(material_res),
                atlas_stride(atlas_stride),
                staging_buffer(staging_buffer),
                staging_buffer_size(staging_buffer_size),
                vertex_count(vertex_count),
                mapped_buffer(mapped_buffer) {
            }
    };
}

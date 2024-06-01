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

#include "internal/render_opengl_legacy/types.hpp"

#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Material;

    struct ProcessedRenderObject;

    struct RenderBucket {
        friend class PoolAllocator;

        const Resource &material_res;
        const Vector2f atlas_stride;
        const uint32_t z_index;
        const float light_opacity;

        std::vector<ProcessedRenderObject *> objects;
        buffer_handle_t vertex_buffer;
        buffer_handle_t anim_frame_buffer;
        void *anim_frame_buffer_staging;
        array_handle_t vertex_array;
        size_t vertex_count;

        bool needs_rebuild;

        static RenderBucket &create(const Resource &material_res, const Vector2f &atlas_stride, uint32_t z_index,
                float light_opacity);

        ~RenderBucket(void);

      private:
        RenderBucket(const Resource &material_res, const Vector2f &atlas_stride, uint32_t z_index,
                float light_opacity) :
            material_res(material_res),
            atlas_stride(atlas_stride),
            z_index(z_index),
            light_opacity(light_opacity),
            objects(),
            vertex_buffer(0),
            anim_frame_buffer_staging(nullptr),
            vertex_array(0),
            vertex_count(0),
            needs_rebuild(true) {
        }
    };
}

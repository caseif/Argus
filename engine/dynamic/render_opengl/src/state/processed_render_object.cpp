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

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/resman/resource.hpp"

#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"

#include <cstddef>

namespace argus {
    static PoolAllocator g_obj_pool(sizeof(ProcessedRenderObject));

    ProcessedRenderObject &ProcessedRenderObject::create(const Resource &material_res, const Vector2f &atlas_stride,
            uint32_t z_index, float light_opacity, buffer_handle_t staging_buffer, size_t staging_buffer_size,
            size_t vertex_count, void *mapped_buffer) {
        return g_obj_pool.construct<ProcessedRenderObject>(material_res, atlas_stride, z_index, light_opacity,
                staging_buffer, staging_buffer_size, vertex_count, mapped_buffer);
    }

    ProcessedRenderObject::~ProcessedRenderObject(void) {
        material_res.release();
        g_obj_pool.free(this);
    }
}

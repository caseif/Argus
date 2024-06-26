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

#include "argus/lowlevel/memory.hpp"

#include "internal/render_opengl_legacy/state/render_bucket.hpp"

namespace argus {
    // forward declarations
    class Resource;

    static PoolAllocator g_bucket_pool(sizeof(RenderBucket));

    RenderBucket &RenderBucket::create(const Resource &material_res, const Vector2f &atlas_stride, uint32_t z_index,
            float light_opacity) {
        return g_bucket_pool.construct<RenderBucket>(material_res, atlas_stride, z_index, light_opacity);
    }

    RenderBucket::~RenderBucket(void) {
        if (this->anim_frame_buffer_staging != nullptr) {
            free(this->anim_frame_buffer_staging);
        }
        g_bucket_pool.free(this);
    }
}

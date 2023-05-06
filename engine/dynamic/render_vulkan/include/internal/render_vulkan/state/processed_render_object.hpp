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

#include "argus/resman/resource.hpp"

#include "internal/render_vulkan/util/buffer.hpp"

#include <cstddef>
#include <cstdint>

namespace argus {
    struct ProcessedRenderObject {
      public:
        const Resource &material_res;
        const Vector2f atlas_stride;
        const uint32_t z_index;
        uint32_t vertex_count;

        Vector2u anim_frame;

        BufferInfo staging_buffer{};
        bool newly_created{};
        bool visited{};
        bool updated{};
        bool anim_frame_updated{};

        static ProcessedRenderObject &create(const Resource &material_res, const Vector2f &atlas_stride,
                uint32_t z_index, uint32_t vertex_count);

        ProcessedRenderObject(ProcessedRenderObject &) = delete;

        void destroy(void);

      private:
        ProcessedRenderObject(const Resource &material_res, const Vector2f &atlas_stride, uint32_t z_index,
                uint32_t vertex_count);

        ~ProcessedRenderObject();
    };
}

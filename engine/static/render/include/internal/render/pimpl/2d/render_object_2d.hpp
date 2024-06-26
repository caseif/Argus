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

#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include <utility>
#include <vector>

namespace argus {
    struct pimpl_RenderObject2D {
        Handle handle;
        const RenderGroup2D &parent_group;
        const std::string material;
        std::vector<RenderPrim2D> primitives;
        Vector2f anchor_point;
        Vector2f atlas_stride;
        uint32_t z_index;
        Dirtiable<float> light_opacity;

        Transform2D transform;
        Dirtiable<Vector2u> active_frame;

        uint16_t version;

        pimpl_RenderObject2D(Handle handle, const RenderGroup2D &parent_group,
                std::string material, const std::vector<RenderPrim2D> &primitives, const Vector2f &anchor_point,
                const Vector2f &atlas_stride, uint32_t z_index, float light_opacity, Transform2D transform):
            handle(handle),
            parent_group(parent_group),
            material(std::move(material)),
            primitives(primitives),
            anchor_point(anchor_point),
            atlas_stride(atlas_stride),
            z_index(z_index),
            light_opacity(light_opacity),
            transform(std::move(transform)),
            active_frame({ 0, 0 }),
            version(1) {
        }

        pimpl_RenderObject2D(const pimpl_RenderObject2D &) = default;

        pimpl_RenderObject2D(pimpl_RenderObject2D &&) = delete;
    };
}

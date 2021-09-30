/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module resman
#include "argus/resman/resource.hpp"

// module render
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include <vector>

namespace argus {
    struct pimpl_RenderObject2D {
        const RenderGroup2D &parent_group;
        const std::string material;
        std::vector<RenderPrim2D> primitives;
        Transform2D transform;

        pimpl_RenderObject2D(const RenderGroup2D &parent_group, const std::string &material,
                const std::vector<RenderPrim2D> &primitives, const Transform2D &transform):
            parent_group(parent_group),
            material(material),
            primitives(primitives),
            transform(transform) {
        }

        pimpl_RenderObject2D(const pimpl_RenderObject2D&) = default;

        pimpl_RenderObject2D(pimpl_RenderObject2D&&) = delete;
    };
}
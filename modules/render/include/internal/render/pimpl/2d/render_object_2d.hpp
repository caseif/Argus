/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/render_object.hpp"
#include "argus/render/render_prim.hpp"

#include <vector>

namespace argus {
    struct pimpl_RenderObject {
        const RenderGroup &parent_group;
        const Material &material;
        std::vector<RenderPrim> primitives;
        Transform transform;

        pimpl_RenderObject(const RenderGroup &parent_group, const Material &material,
                const std::vector<RenderPrim> &primitives, Transform &transform):
            parent_group(parent_group),
            material(material),
            primitives(primitives),
            transform(transform) {
        }

        pimpl_RenderObject(const pimpl_RenderObject&) = default;

        pimpl_RenderObject(pimpl_RenderObject&&) = delete;
    };
}

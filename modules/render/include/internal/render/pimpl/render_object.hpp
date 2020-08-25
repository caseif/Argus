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
#include "internal/render/types.hpp"

#include <vector>

namespace argus {
    struct pimpl_RenderObject {
        RenderLayer &parent_layer;
        RenderGroup *const parent_group;
        const Material &material;
        std::vector<RenderPrim> primitives;
        Transform transform;

        pimpl_RenderObject(RenderLayer &parent_layer, RenderGroup *const parent_group, const Material &material,
                const std::vector<RenderPrim> &primitives, Transform &transform):
            parent_layer(parent_layer),
            parent_group(parent_group),
            material(material),
            primitives(primitives),
            transform(transform) {
        }
    };
}

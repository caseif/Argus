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
#include "argus/render/render_group.hpp"

#include <map>
#include <vector>

namespace argus {
    struct pimpl_RenderGroup {
        const RenderLayer &parent_layer;
        RenderGroup *const parent_group;
        Transform transform;
        std::vector<RenderGroup*> child_groups;
        std::vector<RenderObject*> child_objects;

        pimpl_RenderGroup(const RenderLayer &parent_layer, RenderGroup *const parent_group, Transform &transform):
                parent_layer(parent_layer),
                parent_group(parent_group),
                transform(transform) {
        }

        pimpl_RenderGroup(const RenderLayer &parent_layer, RenderGroup *const parent_group, Transform &&transform):
                parent_layer(parent_layer),
                parent_group(parent_group),
                transform(transform) {
        }

        pimpl_RenderGroup(const RenderLayer &parent_layer, RenderGroup *const parent_group):
                parent_layer(parent_layer),
                parent_group(parent_group) {
        }
    };
}

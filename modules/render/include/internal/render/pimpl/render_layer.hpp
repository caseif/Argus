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
#include "argus/render/render_layer.hpp"

#include <vector>

namespace argus {
    struct pimpl_RenderLayer {
        const Renderer &parent_renderer;
        Transform transform;
        const int index;
        std::vector<RenderGroup*> child_groups;
        std::vector<RenderObject*> child_objects;
        
        pimpl_RenderLayer(const Renderer &parent, Transform &transform, const int index):
                parent_renderer(parent),
                transform(transform),
                index(index) {
        }
    };
}

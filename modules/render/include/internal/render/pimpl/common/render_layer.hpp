/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/common/render_layer.hpp"

#include <vector>

namespace argus {
    struct pimpl_RenderLayer {
        const Renderer &parent_renderer;
        Transform2D transform;
        const int index;
        
        pimpl_RenderLayer(const Renderer &renderer, const RenderLayer &layer, Transform2D &transform, const int index):
                parent_renderer(renderer),
                transform(transform),
                index(index) {
        }

        pimpl_RenderLayer(const pimpl_RenderLayer&) = default;

        pimpl_RenderLayer(pimpl_RenderLayer&&) = delete;
    };
}

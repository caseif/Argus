/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/render/2d/render_group_2d.hpp"
#include "internal/render/pimpl/common/render_layer.hpp"

namespace argus {
    // forward declarations
    struct pimpl_RenderLayer;

    struct pimpl_RenderLayer2D : public pimpl_RenderLayer {
        RenderGroup2D root_group;
        
        pimpl_RenderLayer2D(const Renderer &renderer, const RenderLayer2D &layer, Transform2D &transform,
                const int index):
            pimpl_RenderLayer(renderer, dynamic_cast<const RenderLayer&>(layer), transform, index),
            root_group(layer, nullptr) {
        }

        pimpl_RenderLayer2D(const pimpl_RenderLayer2D&) = default;

        pimpl_RenderLayer2D(pimpl_RenderLayer2D&&) = delete;
    };
}

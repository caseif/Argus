/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module render
#include "argus/render/common/render_layer.hpp"
#include "argus/render/common/render_layer_type.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/pimpl/common/render_layer.hpp"

namespace argus {
    class Renderer;

    RenderLayer::RenderLayer(RenderLayerType type):
        type(type) {
    }

    RenderLayer::~RenderLayer(void) {
    }

    const Renderer &RenderLayer::get_parent_renderer(void) const {
        return get_pimpl()->parent_renderer;
    }

    Transform2D &RenderLayer::get_transform(void) const {
        return get_pimpl()->transform;
    }

    void RenderLayer::set_transform(Transform2D &transform) {
        get_pimpl()->transform = transform;
    }
}

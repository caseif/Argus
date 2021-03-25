/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include <cstdio>

// module render_opengl
#include "internal/render_opengl/layer_state.hpp"
#include "internal/render_opengl/processed_render_object.hpp"

namespace argus {
    // forward declarations
    class RenderLayer;

    struct RendererState;

    LayerState::LayerState(RendererState &parent_state, RenderLayer &layer):
            parent_state(parent_state),
            layer(layer),
            framebuffer(0),
            frame_texture(0) {
    }

    LayerState::~LayerState(void) {
    }

    Layer2DState::Layer2DState(RendererState &parent_state, RenderLayer &layer):
            LayerState(parent_state, layer) {
    }

    Layer2DState::~Layer2DState(void) {
        for (auto &obj : this->processed_objs) {
            obj.second->~ProcessedRenderObject();
        }
    }
}

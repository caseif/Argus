/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

// module render_opengl
#include "internal/render_opengl/state/layer_state.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"

#include <cstdio>

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
        for (auto &bucket : render_buckets) {
            bucket.second->~RenderBucket();
        }
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

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module render_opengl
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"
#include "internal/render_opengl/state/scene_state.hpp"

#include <map>
#include <string>
#include <utility>

namespace argus {
    // forward declarations
    class Scene;

    struct RendererState;

    SceneState::SceneState(RendererState &parent_state, Scene &scene):
            parent_state(parent_state),
            scene(scene),
            framebuffer(0),
            frame_texture(0) {
    }

    SceneState::~SceneState(void) {
        for (auto &bucket : render_buckets) {
            bucket.second->~RenderBucket();
        }
    }

    Scene2DState::Scene2DState(RendererState &parent_state, Scene &scene):
            SceneState(parent_state, scene) {
    }

    Scene2DState::~Scene2DState(void) {
        for (auto &obj : this->processed_objs) {
            obj.second->~ProcessedRenderObject();
        }
    }
}

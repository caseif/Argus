/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "internal/render_opengles/state/processed_render_object.hpp"
#include "internal/render_opengles/state/render_bucket.hpp"
#include "internal/render_opengles/state/scene_state.hpp"

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
            reinterpret_cast<ProcessedRenderObject*>(obj.second)->~ProcessedRenderObject();
        }
    }
}

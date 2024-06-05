/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/render/common/scene.hpp"

#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"

namespace argus {
    SceneState::SceneState(RendererState &parent_state, Scene &scene):
        parent_state(parent_state),
        scene(scene),
        visited(false) {
    }

    SceneState::SceneState(SceneState &&rhs) noexcept = default;

    SceneState::~SceneState(void) = default;

    Scene2DState::Scene2DState(RendererState &parent_state, Scene &scene):
        SceneState(parent_state, scene) {
    }

    Scene2DState::Scene2DState(Scene2DState &&rhs) noexcept = default;

    Scene2DState::~Scene2DState(void) = default;
}

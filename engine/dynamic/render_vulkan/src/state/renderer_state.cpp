/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/logging.hpp"

#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/common/scene.hpp"

#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/state/scene_state.hpp"

namespace argus {
    SceneState &RendererState::get_scene_state(Scene &scene) {
        switch (scene.type) {
            case SceneType::TwoD: {
                auto &scene_2d = reinterpret_cast<const Scene2D &>(scene);
                auto it = this->scene_states_2d.find(&scene_2d);
                if (it != this->scene_states_2d.cend()) {
                    return it->second;
                }

                Logger::default_logger().fatal("Failed to get scene state");
            }
            case SceneType::ThreeD: {
                Logger::default_logger().fatal("Unimplemented scene type");
            }
            default: {
                Logger::default_logger().fatal("Unrecognized scene type");
            }
        }
    }

    ViewportState &RendererState::get_viewport_state(AttachedViewport &viewport) {
        switch (viewport.type) {
            case SceneType::TwoD: {
                auto &viewport_2d = reinterpret_cast<AttachedViewport2D &>(viewport);
                auto it = this->viewport_states_2d.find(&viewport_2d);
                if (it != this->viewport_states_2d.cend()) {
                    return it->second;
                }

                Logger::default_logger().fatal("Failed to get viewport state");
            }
            case SceneType::ThreeD: {
                Logger::default_logger().fatal("Unimplemented viewport type");
            }
            default: {
                Logger::default_logger().fatal("Unrecognized viewport type");
            }
        }
    }
}

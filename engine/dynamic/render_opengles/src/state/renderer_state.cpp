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

#include "internal/render_opengles/state/renderer_state.hpp"

#include "argus/lowlevel/logging.hpp"

#include "argus/resman/resource.hpp"

#include "argus/render/common/scene.hpp"

#include "internal/render_opengles/renderer/shader_mgmt.hpp"
#include "internal/render_opengles/renderer/texture_mgmt.hpp"
#include "internal/render_opengles/state/scene_state.hpp"
#include "internal/render_opengles/state/viewport_state.hpp"
#include "internal/render_opengles/types.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    class Canvas;
    class Scene2D;

    RendererState::RendererState(GLESRenderer &renderer):
        renderer(renderer) {
    }

    RendererState::~RendererState(void) {
        // the destructor will be automatically called for each scene state since we're storing the states directly
        this->scene_states_2d.clear();

        for (auto &program : this->linked_programs) {
            deinit_program(program.second.handle);
        }
        this->linked_programs.clear();

        for (auto &shader : this->compiled_shaders) {
            deinit_shader(shader.second);
        }
        this->compiled_shaders.clear();

        for (auto &texture : this->prepared_textures) {
            deinit_texture(texture.second);
        }
        this->prepared_textures.clear();

        for (auto *res : this->intrinsic_resources) {
            res->release();
        }
    }
    
    SceneState &RendererState::get_scene_state(Scene &scene, bool create) {
        switch (scene.type) {
            case SceneType::TwoD: {
                auto &scene_2d = reinterpret_cast<const Scene2D&>(scene);
                auto it = this->scene_states_2d.find(&scene_2d);
                if (it != this->scene_states_2d.cend()) {
                    return it->second;
                }

                if (!create) {
                    Logger::default_logger().fatal("Failed to get scene state");
                }

                auto insert_res = this->scene_states_2d.try_emplace(&scene_2d, *this, scene);
                if (!insert_res.second) {
                    Logger::default_logger().fatal("Failed to create new scene state");
                }

                return insert_res.first->second;
            }
            case SceneType::ThreeD: {
                Logger::default_logger().fatal("Unimplemented scene type");
            }
            default: {
                Logger::default_logger().fatal("Unrecognized scene type");
            }
        }
    }

    ViewportState &RendererState::get_viewport_state(AttachedViewport &viewport, bool create) {
        switch (viewport.type) {
            case SceneType::TwoD: {
                auto &viewport_2d = reinterpret_cast<AttachedViewport2D&>(viewport);
                auto it = this->viewport_states_2d.find(&viewport_2d);
                if (it != this->viewport_states_2d.cend()) {
                    return it->second;
                }

                if (!create) {
                    Logger::default_logger().fatal("Failed to get viewport state");
                }

                Viewport2DState state(*this, &viewport_2d);
                auto insert_res = this->viewport_states_2d.insert({&viewport_2d, state});
                if (!insert_res.second) {
                    Logger::default_logger().fatal("Failed to create new viewport state");
                }

                return insert_res.first->second;
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

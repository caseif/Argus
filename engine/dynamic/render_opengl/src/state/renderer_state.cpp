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

#include "internal/lowlevel/logging.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"
#include "internal/render_opengl/state/scene_state.hpp"

#include "argus/resman/resource.hpp"

#include "argus/render/common/scene.hpp"

#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/shader_mgmt.hpp"
#include "internal/render_opengl/renderer/texture_mgmt.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    class Canvas;
    class Scene2D;

    RendererState::RendererState(GLRenderer &renderer):
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
                    _ARGUS_FATAL("Failed to get scene state");
                }

                Scene2DState state = Scene2DState(*this, scene);
                auto insert_res = this->scene_states_2d.insert({&scene_2d, state});
                if (!insert_res.second) {
                    _ARGUS_FATAL("Failed to create new scene state");
                }

                return insert_res.first->second;
            }
            case SceneType::ThreeD: {
                _ARGUS_FATAL("Unimplemented scene type");
            }
            default: {
                _ARGUS_FATAL("Unrecognized scene type");
            }
        }
    }
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"
#include "internal/render_opengl/state/scene_state.hpp"

// module resman
#include "argus/resman/resource.hpp"

// module render
#include "argus/render/common/scene.hpp"

// module render_opengl
#include "internal/render_opengl/types.hpp"

#include "aglet/aglet.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    class Renderer;
    class Scene2D;

    RendererState::RendererState(Renderer &renderer):
        renderer(renderer) {
    }

    RendererState::~RendererState(void) {
        // the destructor will be automatically called for each scene state since we're storing the states directly
        this->scene_states_2d.clear();

        for (auto &program : this->linked_programs) {
            glDeleteProgram(program.second.handle);
        }
        this->linked_programs.clear();

        for (auto &shader : this->compiled_shaders) {
            glDeleteShader(shader.second);
        }
        this->compiled_shaders.clear();

        for (auto &texture : this->prepared_textures) {
            glDeleteTextures(1, &texture.second);
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

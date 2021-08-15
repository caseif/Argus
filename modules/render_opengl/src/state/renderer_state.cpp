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

// module render
#include "argus/render/common/render_layer.hpp"
#include "argus/render/common/render_layer_type.hpp"

// module render_opengl
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/state/layer_state.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

#include <map>
#include <vector>

namespace argus {
    // forward declarations
    class Renderer;
    class RenderLayer2D;

    RendererState::RendererState(Renderer &renderer):
        renderer(renderer) {
    }

    RendererState::~RendererState(void) {
        // the destructor will be automatically called for each layer state since we're storing the states directly
        this->layer_states_2d.clear();

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
    
    LayerState &RendererState::get_layer_state(RenderLayer &layer, bool create) {
        switch (layer.type) {
            case RenderLayerType::Render2D: {
                auto &layer_2d = reinterpret_cast<const RenderLayer2D&>(layer);
                auto it = this->layer_states_2d.find(&layer_2d);
                if (it != this->layer_states_2d.cend()) {
                    return it->second;
                }

                if (!create) {
                    _ARGUS_FATAL("Failed to get layer state");
                }

                Layer2DState state = Layer2DState(*this, layer);
                auto insert_res = this->layer_states_2d.insert({&layer_2d, state});
                if (!insert_res.second) {
                    _ARGUS_FATAL("Failed to create new layer state");
                }

                return insert_res.first->second;
            }
            case RenderLayerType::Render3D: {
                _ARGUS_FATAL("Unimplemented layer type");
            }
            default: {
                _ARGUS_FATAL("Unrecognized layer type");
            }
        }
    }
}

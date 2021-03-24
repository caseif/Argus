/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/common/render_layer_type.hpp"
// module render_opengl
#include "internal/render_opengl/globals.hpp"

#include <map>
#include <string>
#include <utility>

namespace argus {
    // forward declarations
    class ProcessedRenderObject;
    class RenderObject2D;
    
    class RenderBucket;

    struct LinkedProgram {
        program_handle_t handle;
        uniform_location_t view_matrix_uniform_loc;
    };

    struct LayerState {
        //TODO: this map should be sorted or otherwise bucketed by shader and texture
        std::map<const Material*, RenderBucket*> render_buckets;

        mat4_flat_t view_matrix;

        buffer_handle_t framebuffer;
        texture_handle_t frame_texture;

        LayerState(void):
            framebuffer(0),
            frame_texture(0) {
        }
    };

    struct Layer2DState : public LayerState {
        std::map<const RenderObject2D*, ProcessedRenderObject*> processed_objs;
    };

    struct RendererState {
        Renderer &renderer;

        std::map<const RenderLayer2D*, Layer2DState> layer_states_2d;
        std::vector<LayerState*> all_layer_states;
        std::map<const TextureData*, texture_handle_t> prepared_textures;
        std::map<const Shader*, shader_handle_t> compiled_shaders;
        std::map<const Material*, LinkedProgram> linked_programs;

        buffer_handle_t frame_vbo;
        array_handle_t frame_vao;
        program_handle_t frame_program;
        shader_handle_t frame_vert_shader;
        shader_handle_t frame_frag_shader;

        RendererState(Renderer &renderer):
            renderer(renderer) {
        }
        
        LayerState &get_layer_state(const RenderLayer &layer, bool create = false) {
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

                    Layer2DState state = Layer2DState();
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
    };
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module resman
#include "argus/resman/resource.hpp"

// module render_opengl
#include "internal/render_opengl/state/layer_state.hpp"
#include "internal/render_opengl/types.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    class Renderer;
    class RenderLayer;
    class Shader;
    class TextureData;
    class RenderLayer2D;
    class RenderObject2D;

    struct ProcessedRenderObject;
    struct RenderBucket;

    struct LinkedProgram {
        program_handle_t handle;
        uniform_location_t view_matrix_uniform_loc;
        std::vector<shader_handle_t> attached_shaders;
    };

    struct RendererState {
        Renderer &renderer;

        std::vector<Resource*> intrinsic_resources;

        std::map<const RenderLayer2D*, Layer2DState> layer_states_2d;
        std::vector<LayerState*> all_layer_states;
        std::map<std::string, texture_handle_t> prepared_textures;
        std::map<std::string, shader_handle_t> compiled_shaders;
        std::map<std::string, LinkedProgram> linked_programs;

        buffer_handle_t frame_vbo;
        array_handle_t frame_vao;
        program_handle_t frame_program;
        shader_handle_t frame_vert_shader;
        shader_handle_t frame_frag_shader;

        RendererState(Renderer &renderer);

        ~RendererState(void);
        
        LayerState &get_layer_state(RenderLayer &layer, bool create = false);
    };
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render_opengl
#include "internal/render_opengl/globals.hpp"

#include <map>
#include <string>
#include <utility>

namespace argus {
    // forward declarations
    class ProcessedRenderObject;
    class RenderObject;
    
    class RenderBucket;

    struct LinkedProgram {
        program_handle_t handle;
        uniform_location_t view_matrix_uniform_loc;
    };

    struct LayerState {
        std::map<const RenderObject*, ProcessedRenderObject*> processed_objs;
        //TODO: this map should be sorted or otherwise bucketed by shader and texture
        std::map<const Material*, RenderBucket*> render_buckets;

        mat4_flat_t view_matrix;

        LayerState(void) {
        }
    };

    struct RendererState {
        std::map<const RenderLayer*, LayerState> layer_states;
        std::map<const TextureData*, texture_handle_t> prepared_textures;
        std::map<const Shader*, shader_handle_t> compiled_shaders;
        std::map<const Material*, LinkedProgram> linked_programs;

        RendererState(void) {
        }
    };
}

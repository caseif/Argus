/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#pragma once

#include "argus/lowlevel/refcountable.hpp"

#include "argus/resman/resource.hpp"

#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/state/scene_state.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    class Canvas;
    class Scene;
    class Shader;
    class TextureData;

    class RenderObject2D;
    class Scene2D;

    class GLESRenderer;
    struct ProcessedRenderObject;
    struct RenderBucket;

    struct LinkedProgram {
        program_handle_t handle;
        std::optional<attribute_location_t> attr_position_loc;
        std::optional<attribute_location_t> attr_normal_loc;
        std::optional<attribute_location_t> attr_color_loc;
        std::optional<attribute_location_t> attr_texcoord_loc;
        uniform_location_t view_matrix_uniform_loc;

        LinkedProgram(program_handle_t handle, attribute_location_t attr_pos, attribute_location_t attr_norm,
            attribute_location_t attr_color, attribute_location_t attr_tc, uniform_location_t unif_vm):
            handle(handle),
            attr_position_loc(attr_pos != -1 ? std::optional(attr_pos) : std::nullopt),
            attr_normal_loc(attr_norm != -1 ? std::optional(attr_norm) : std::nullopt),
            attr_color_loc(attr_color != -1 ? std::optional(attr_color) : std::nullopt),
            attr_texcoord_loc(attr_tc != -1 ? std::optional(attr_tc) : std::nullopt),
            view_matrix_uniform_loc(unif_vm) {
        }
    };

    struct RendererState {
        GLESRenderer &renderer;

        std::vector<Resource*> intrinsic_resources;

        std::map<const Scene2D*, Scene2DState> scene_states_2d;
        std::vector<SceneState*> all_scene_states;
        std::map<std::string, RefCountable<texture_handle_t>> prepared_textures;
        std::map<std::string, std::string> material_textures;
        std::map<std::string, shader_handle_t> compiled_shaders;
        std::map<std::string, LinkedProgram> linked_programs;

        buffer_handle_t frame_vbo;
        array_handle_t frame_vao;
        program_handle_t frame_program;
        shader_handle_t frame_vert_shader;
        shader_handle_t frame_frag_shader;

        RendererState(GLESRenderer &renderer);

        ~RendererState(void);
        
        SceneState &get_scene_state(Scene &scene, bool create = false);
    };
}

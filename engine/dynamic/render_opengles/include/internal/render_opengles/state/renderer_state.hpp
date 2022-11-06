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

#include "argus/render/util/linked_program.hpp"

#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/state/scene_state.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    struct AttachedViewport;
    struct AttachedViewport2D;
    class Canvas;
    class Scene;
    class Shader;
    class TextureData;

    class RenderObject2D;
    class Scene2D;

    class GLESRenderer;
    struct ProcessedRenderObject;
    struct RenderBucket;

    struct ViewportState;

    struct RendererState {
        GLESRenderer &renderer;

        std::vector<Resource*> intrinsic_resources;

        std::map<const Scene2D*, Scene2DState> scene_states_2d;
        std::vector<SceneState*> all_scene_states;
        std::map<const AttachedViewport2D*, ViewportState> viewport_states_2d;
        std::map<std::string, RefCountable<texture_handle_t>> prepared_textures;
        std::map<std::string, std::string> material_textures;
        std::map<std::string, shader_handle_t> compiled_shaders;
        std::map<std::string, LinkedProgram> linked_programs;

        std::map<std::string, LinkedProgram> postfx_programs;

        buffer_handle_t frame_vbo;
        array_handle_t frame_vao;
        std::optional<LinkedProgram> frame_program;

        RendererState(GLESRenderer &renderer);

        ~RendererState(void);

        SceneState &get_scene_state(Scene &scene, bool create = false);

        ViewportState &get_viewport_state(AttachedViewport &viewport, bool create = false);
    };
}

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

#include "argus/lowlevel/math.hpp"

#include "argus/render/util/object_processor.hpp"

#include "internal/render_opengl/state/linked_program.hpp"
#include "internal/render_opengl/types.hpp"

#include <map>
#include <string>

namespace argus {
    // forward declarations
    class Material;
    class RenderObject2D;
    class Scene;

    struct ProcessedRenderObject;
    struct RenderBucket;
    struct RendererState;

    struct SceneState {
        RendererState &parent_state;

        Scene &scene;

        //TODO: this map should be sorted or otherwise bucketed by shader and texture
        std::map<std::string, RenderBucket*> render_buckets;

        std::map<std::string, LinkedProgram> postfx_programs;

        Matrix4 view_matrix;

        buffer_handle_t front_fb;
        buffer_handle_t back_fb;

        texture_handle_t front_frame_tex;
        texture_handle_t back_frame_tex;

        SceneState(RendererState &parent_state, Scene &scene);

        ~SceneState(void);
    };

    struct Scene2DState : public SceneState {
        std::map<const RenderObject2D*, ProcessedRenderObject2DPtr> processed_objs;

        Scene2DState(RendererState &parent_state, Scene &scene);

        ~Scene2DState(void);
    };
}

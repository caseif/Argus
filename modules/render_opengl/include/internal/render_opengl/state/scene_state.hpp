/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module render_opengl
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

        Matrix4 view_matrix;

        buffer_handle_t framebuffer;
        texture_handle_t frame_texture;

        SceneState(RendererState &parent_state, Scene &scene);

        ~SceneState(void);
    };

    struct Scene2DState : public SceneState {
        std::map<const RenderObject2D*, ProcessedRenderObject*> processed_objs;

        Scene2DState(RendererState &parent_state, Scene &scene);

        ~Scene2DState(void);
    };
}
/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

namespace argus {
    // forward declarations
    class Scene2D;

    struct RendererState;
    struct Scene2DState;

    void render_scene_2d(Scene2D &scene, RendererState &renderer_state, Scene2DState &scene_state);
}

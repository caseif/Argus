/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "aglet/aglet.h"

namespace argus {
    // forward declarations
    struct RendererState;
    struct SceneState;

    void draw_scene_to_framebuffer(SceneState &scene_state);

    void draw_framebuffer_to_screen(SceneState &scene_state);

    void setup_framebuffer(RendererState &state);
}

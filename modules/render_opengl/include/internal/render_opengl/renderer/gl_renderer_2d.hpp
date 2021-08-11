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
    struct Layer2DState;
    struct RendererState;
    class RenderLayer2D;

    void render_layer_2d(RenderLayer2D &layer, RendererState &renderer_state, Layer2DState &layer_state);
}

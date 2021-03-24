#pragma once

namespace argus {
    // forward declarations
    class Layer2DState;
    class RendererState;
    class RenderLayer2D;

    void render_layer_2d(RenderLayer2D &layer, RendererState &renderer_state, Layer2DState &layer_state);
}

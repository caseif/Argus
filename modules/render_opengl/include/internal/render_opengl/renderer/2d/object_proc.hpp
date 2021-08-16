#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

namespace argus {
    // forward declarations
    struct Layer2DState;
    struct ProcessedRenderObject;
    class RenderObject2D;

    void process_object_2d(Layer2DState &layer_state, const RenderObject2D &object, const mat4_flat_t &transform);

    void deinit_object_2d(ProcessedRenderObject &obj);
}

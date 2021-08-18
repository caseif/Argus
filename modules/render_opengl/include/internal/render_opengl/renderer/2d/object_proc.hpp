#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

namespace argus {
    // forward declarations
    class RenderObject2D;

    struct ProcessedRenderObject;
    struct Scene2DState;

    void process_object_2d(Scene2DState &scene_state, const RenderObject2D &object, const mat4_flat_t &transform);

    void deinit_object_2d(ProcessedRenderObject &obj);
}

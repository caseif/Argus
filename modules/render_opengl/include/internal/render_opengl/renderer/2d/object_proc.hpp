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

namespace argus {
    // forward declarations
    class RenderObject2D;

    struct ProcessedRenderObject;
    struct Scene2DState;

    void process_object_2d(Scene2DState &scene_state, const RenderObject2D &object, const Matrix4 &transform);

    void deinit_object_2d(ProcessedRenderObject &obj);
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

namespace argus {
    // forward declarations
    class RenderObject2D;

    struct ProcessedRenderObject;
    struct RendererState;
    struct Scene2DState;

    ProcessedRenderObject2DPtr create_processed_object_2d(const RenderObject2D &object, const Matrix4 &transform,
            void *scene_state_ptr);

    void update_processed_object_2d(const RenderObject2D &object, ProcessedRenderObject2DPtr proc_obj_ptr,
            const Matrix4 &transform, bool is_transform_dirty, void *scene_state_ptr);

    void deinit_object_2d(const RendererState &state, ProcessedRenderObject &obj);
}

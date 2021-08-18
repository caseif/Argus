/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

// module resman
#include "argus/resman/resource.hpp"

// module render_opengl
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/state/processed_render_object.hpp"

#include <cstddef>

namespace argus {
    static AllocPool g_obj_pool(sizeof(ProcessedRenderObject));

    ProcessedRenderObject &ProcessedRenderObject::create(const Resource &material_res, const mat4_flat_t abs_transform,
            const buffer_handle_t vertex_buffer,
            const size_t vertex_buffer_size, const size_t vertex_count) {
        return g_obj_pool.construct<ProcessedRenderObject>(material_res, abs_transform, vertex_buffer,
                                                           vertex_buffer_size, vertex_count);
    }

    ProcessedRenderObject::~ProcessedRenderObject(void) {
        material_res.release();
        g_obj_pool.free(this);
    }
}

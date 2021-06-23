/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include <cstddef>
#include <cstdio>


// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

// module render_opengl
#include "internal/render_opengl/globals.hpp"
#include "internal/render_opengl/processed_render_object.hpp"

namespace argus {
    // forward declarations
    class Material;

    static AllocPool g_obj_pool(sizeof(ProcessedRenderObject));

    ProcessedRenderObject &ProcessedRenderObject::create(const Material &material, const mat4_flat_t abs_transform,
            const buffer_handle_t vertex_buffer, const size_t vertex_buffer_size, const size_t vertex_count) {
        return g_obj_pool.construct<ProcessedRenderObject>(material, abs_transform, vertex_buffer, vertex_buffer_size,
                vertex_count);
    }
        
    ProcessedRenderObject::~ProcessedRenderObject(void) {
        g_obj_pool.free(this);
    }
}
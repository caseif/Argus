/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/transform.hpp"

// module render_opengl
#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/globals.hpp"

#include <numeric>

#include <cstring>

namespace argus {
    // forward definitions
    class Material;
    class RenderObject;

    struct ProcessedRenderObject {
        const RenderObject *orig;
        const Material *material;
        float abs_transform[16];
        buffer_handle_t vertex_buffer;
        size_t vertex_buffer_size;
        size_t vertex_count;
        bool visited;
        bool updated;

        ProcessedRenderObject(const RenderObject &orig, const Material &material, const mat4_flat_t abs_transform,
                const buffer_handle_t vertex_buffer, const size_t vertex_buffer_size): 
            orig(&orig),
            material(&material),
            vertex_buffer(vertex_buffer),
            vertex_buffer_size(vertex_buffer_size),
            vertex_count(std::accumulate(orig.get_primitives().cbegin(), orig.get_primitives().cend(), 0,
                    [](const size_t acc, const RenderPrim &prim) {
                        return acc + prim.get_vertex_count();
                    })) {
            memcpy(this->abs_transform, abs_transform, 16 * sizeof(this->abs_transform[0]));
        }
    };
}

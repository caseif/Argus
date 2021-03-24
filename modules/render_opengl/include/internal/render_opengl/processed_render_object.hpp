/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/common/transform.hpp"

// module render_opengl
#include "internal/render_opengl/glfw_include.hpp"
#include "internal/render_opengl/globals.hpp"

#include <numeric>

#include <cstddef>
#include <cstring>

namespace argus {
    // forward definitions
    class Material;
    class RenderObject2D;

    struct ProcessedRenderObject {
        friend class AllocPool;

        const Material *material;
        float abs_transform[16];
        buffer_handle_t vertex_buffer;
        size_t vertex_buffer_size;
        size_t vertex_count;
        bool visited;
        bool updated;

        static ProcessedRenderObject &create(const Material &material, const mat4_flat_t abs_transform,
                const buffer_handle_t vertex_buffer, const size_t vertex_buffer_size, const size_t vertex_count);
        
        void destroy(void);

        private:
            ProcessedRenderObject(const Material &material, const mat4_flat_t abs_transform,
                    const buffer_handle_t vertex_buffer, const size_t vertex_buffer_size, const size_t vertex_count):
                material(&material),
                vertex_buffer(vertex_buffer),
                vertex_buffer_size(vertex_buffer_size),
                vertex_count(vertex_count) {
                memcpy(this->abs_transform, abs_transform, 16 * sizeof(this->abs_transform[0]));
            }

            ~ProcessedRenderObject() {
            }
    };
}

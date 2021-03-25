/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render_opengl
#include "internal/render_opengl/globals.hpp"

#include <vector>

namespace argus {
    // forward declarations
    class Material;
    class ProcessedRenderObject;

    struct RenderBucket {
        friend class AllocPool;

        const Material &material;
        std::vector<ProcessedRenderObject*> objects;
        buffer_handle_t vertex_buffer;
        buffer_handle_t vertex_array;
        size_t vertex_count;

        bool needs_rebuild;

        static RenderBucket &create(const Material &material);

        ~RenderBucket(void);

        private:
            RenderBucket(const Material &material):
                material(material),
                objects(),
                vertex_buffer(0),
                vertex_array(0),
                vertex_count(0),
                needs_rebuild(true) {
            }
    };
}

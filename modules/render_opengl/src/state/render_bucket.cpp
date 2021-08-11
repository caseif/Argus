/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render_opengl
#include "internal/render_opengl/state/processed_render_object.hpp"
#include "internal/render_opengl/state/render_bucket.hpp"

#include <string>

#include <cstdio>

namespace argus {
    // forward declarations
    class Material;

    static AllocPool g_bucket_pool(sizeof(RenderBucket));

    RenderBucket &RenderBucket::create(const Resource &material_res) {
        return g_bucket_pool.construct<RenderBucket>(material_res);
    }

    RenderBucket::~RenderBucket(void) {
        g_bucket_pool.free(this);
    }
}

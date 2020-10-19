/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/config.hpp"
#include "internal/core/dyn_invoke.hpp"
#include "internal/lowlevel/logging.hpp"

// module render
#include "argus/render/renderer.hpp"
#include "internal/render/defines.hpp"

// module render_opengl
#include "internal/render_opengl/gl_renderer.hpp"
#include "internal/render_opengl/globals.hpp"

#include <cstring>

namespace argus {
    mat4_flat_t g_view_matrix;

    RendererImpl *create_opengl_backend() {
        return new GLRenderer();
    }

    static void _setup_view_matrix() {
        auto screen_space = get_engine_config().screen_space;
        
        auto l = screen_space.left;
        auto r = screen_space.right;
        auto b = screen_space.bottom;
        auto t = screen_space.top;
        
        float mat[16] = {
            2 / (r - l), 0, 0, 0,
            0, 2 / (t - b), 0, 0,
            0, 0, 1, 0,
            -(r + l) / (r - l), -(t + b) / (t - b), 0, 1
        };

        memcpy(g_view_matrix, mat, sizeof(mat));
    }

    void update_lifecycle_render_opengl(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PRE_INIT:
                register_module_fn(FN_CREATE_OPENGL_BACKEND, reinterpret_cast<void*>(create_opengl_backend));
                break;
            case LifecycleStage::INIT:
                _setup_view_matrix();
                break;
        }
    }

    REGISTER_ARGUS_MODULE("render_opengl", 4, { "render" }, update_lifecycle_render_opengl);
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module core
#include "argus/core.hpp"
#include "internal/core/dyn_invoke.hpp"
#include "internal/lowlevel/logging.hpp"

// module render
#include "argus/render/renderer.hpp"
#include "internal/render/defines.hpp"

// module render_opengl
#include "internal/render_opengl/gl_renderer.hpp"

namespace argus {
    RendererImpl *create_opengl_backend(Renderer &parent) {
        return new GLRenderer(parent);
    }

    void update_lifecycle_render_opengl(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PRE_INIT:
                register_module_fn(FN_CREATE_OPENGL_BACKEND, reinterpret_cast<void*>(create_opengl_backend));
                break;
        }
    }

    REGISTER_ARGUS_MODULE("render_opengl", 4, { "render" }, update_lifecycle_render_opengl);
}

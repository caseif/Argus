/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/engine_config.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"
#include "internal/core/dyn_invoke.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/module.hpp"

// module resman
#include "argus/resman/resource_manager.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/window.hpp"

// module render
#include "argus/render/common/renderer.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/renderer.hpp"
#include "internal/render/texture_loader.hpp"

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <cstdio>

namespace argus {
    // forward declarations
    class RendererImpl;

    bool g_render_module_initialized = false;

    RendererImpl *g_renderer_impl;

    std::map<Window*, Renderer*> g_renderer_map;

    static RendererImpl &_create_backend_impl() {
        auto backends = get_engine_config().render_backends;

        for (auto backend : backends) {
            switch (backend) {
                case RenderBackend::OPENGL: {
                    auto impl = call_module_fn<RendererImpl*>(std::string(FN_CREATE_OPENGL_BACKEND));
                    _ARGUS_INFO("Selecting OpenGL as graphics backend\n");
                    return *impl;
                }
                case RenderBackend::OPENGLES:
                    _ARGUS_INFO("Graphics backend OpenGL ES is not yet supported\n");
                    break;
                case RenderBackend::VULKAN:
                    _ARGUS_INFO("Graphics backend Vulkan is not yet supported\n");
                    break;
                default:
                    _ARGUS_WARN("Skipping unrecognized graphics backend index %d\n", static_cast<int>(backend));
                    break;
            }
            _ARGUS_INFO("Current graphics backend cannot be selected, continuing to next\n");
        }

        _ARGUS_WARN("Failed to select graphics backend from preference list, defaulting to OpenGL\n");
        return *call_module_fn<RendererImpl*>(std::string(FN_CREATE_OPENGL_BACKEND));
    }

    RendererImpl &get_renderer_impl(void) {
        if (!g_render_module_initialized) {
            throw std::runtime_error("Cannot get renderer implementation before render module is initialized");
        }

        return *g_renderer_impl;
    }

    static void _window_construct_callback(Window &window) {
        auto *renderer = new Renderer(window);
        g_renderer_map.insert({&window, renderer});
    }

    void _update_lifecycle_render(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::INIT: {
                g_renderer_impl = &_create_backend_impl();

                set_window_construct_callback(_window_construct_callback);

                register_event_handler(ArgusEventType::WINDOW, renderer_window_event_callback, TargetThread::RENDER);

                ResourceManager::get_global_resource_manager()
                    .register_loader(RESOURCE_TYPE_TEXTURE_PNG, new PngTextureLoader());

                g_render_module_initialized = true;

                break;
            }
            default:
                break;
        }
    }

    void load_backend_modules(void) {
        //TODO: fail gracefully
        enable_module(MODULE_RENDER_OPENGL);
    }

    void init_module_render(void) {
        register_module({MODULE_RENDER, 3, {"core", "wm", "resman"}, _update_lifecycle_render});

        register_early_init_callback(MODULE_RENDER, load_backend_modules);
    }

}

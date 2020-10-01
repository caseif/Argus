/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/config.hpp"
#include "internal/core/dyn_invoke.hpp"
#include "internal/core/lifecycle.hpp"

// module resman
#include "argus/resman.hpp"

// module render
#include "argus/render/window.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/texture_loader.hpp"

#include "GLFW/glfw3.h"

#include <iterator>
#include <map>
#include <string>
#include <utility>

#include <cstddef>

namespace argus {
    // forward declarations
    class RendererImpl;

    bool g_render_module_initialized = false;

    RendererImpl *g_renderer_impl;

    // maps GLFW window pointers to Window instance pointers
    std::map<GLFWwindow*, Window*> g_window_map;
    size_t g_window_count = 0;

    static void _clean_up(void) {
        // use a copy since Window::destroy modifies the global list
        auto windows_copy = g_window_map;
        // doing this in reverse ensures that child windows are destroyed before their parents
        for (auto it = windows_copy.rbegin();
                it != windows_copy.rend(); it++) {
            it->second->destroy();
        }

        glfwTerminate();

        return;
    }

    static void _poll_events(const TimeDelta delta) {
        glfwPollEvents();
    }

    static void _on_glfw_error(int code, const char *desc) {
        _ARGUS_WARN("GLFW Error: %s", desc);
    }

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

    void _update_lifecycle_render(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PRE_INIT: {
                break;
            }
            case LifecycleStage::INIT: {
                g_renderer_impl = &_create_backend_impl();

                glfwInit();

                glfwSetErrorCallback(_on_glfw_error);

                register_render_callback(_poll_events);

                ResourceManager::get_global_resource_manager()
                    .register_loader(RESOURCE_TYPE_TEXTURE_PNG, new PngTextureLoader());

                g_render_module_initialized = true;

                break;
            }
            case LifecycleStage::DEINIT:
                _clean_up();
                break;
            default:
                break;
        }
    }

    void load_backend_modules(void) {
        //TODO: fail gracefully
        enable_module(MODULE_RENDER_OPENGL);
    }

    void init_module_render(void) {
        register_module({MODULE_RENDER, 3, {"core", "resman"}, _update_lifecycle_render});

        register_early_init_callback(MODULE_RENDER, load_backend_modules);
    }

}

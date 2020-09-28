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
#include <utility>

#include <cstddef>

namespace argus {

    bool g_render_module_initialized = false;

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

    static void poll_events(const TimeDelta delta) {
        glfwPollEvents();
    }

    static void _on_glfw_error(int code, const char *desc) {
        _ARGUS_WARN("GLFW Error: %s", desc);
    }

    void _update_lifecycle_render(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::INIT: {
                glfwInit();

                glfwSetErrorCallback(_on_glfw_error);

                register_render_callback(poll_events);

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

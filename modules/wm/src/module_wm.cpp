/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/time.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/engine.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/window.hpp"

#include "GLFW/glfw3.h"

#include <iterator>
#include <map>
#include <utility>

#include <cstddef>

namespace argus {
    bool g_wm_module_initialized = false;

    // maps GLFW window pointers to Window instance pointers
    std::map<GLFWwindow*, Window*> g_window_map;
    size_t g_window_count = 0;

    static void _clean_up(void) {
        // use a copy since Window destructor modifies the global list
        auto windows_copy = g_window_map;
        // doing this in reverse ensures that child windows are destroyed before their parents
        for (auto it = windows_copy.rbegin();
                it != windows_copy.rend(); it++) {
            delete it->second;
        }

        glfwTerminate();

        return;
    }

    static void _poll_events(const TimeDelta delta) {
        UNUSED(delta);
        glfwPollEvents();
    }

    static void _on_glfw_error(const int code, const char *desc) {
        UNUSED(code);
        _ARGUS_WARN("GLFW Error: %s\n", desc);
    }

    static void _update_lifecycle_wm(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::INIT: {
                glfwInit();

                glfwSetErrorCallback(_on_glfw_error);

                register_render_callback(_poll_events);
                
                register_event_handler(ArgusEventType::WINDOW, window_window_event_callback, TargetThread::RENDER);

                g_wm_module_initialized = true;

                break;
            }
            case LifecycleStage::DEINIT:
                _clean_up();
                break;
            default:
                break;
        }
    }

    void init_module_wm(void) {
        register_module({ModuleWm, 2, {"core"}, _update_lifecycle_wm});
    }

}

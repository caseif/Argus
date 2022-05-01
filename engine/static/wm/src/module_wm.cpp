/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/time.hpp"
#include "internal/lowlevel/logging.hpp"

#include "argus/core/engine.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/display.hpp"
#include "internal/wm/module_wm.hpp"
#include "internal/wm/window.hpp"

#include "GLFW/glfw3.h"

#include <iterator>
#include <map>
#include <string>
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
        _ARGUS_WARN("GLFW Error: %s", desc);
    }

    void update_lifecycle_wm(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
                glfwInit();

                glfwSetErrorCallback(_on_glfw_error);

                _ARGUS_DEBUG("Initialized GLFW");

                register_render_callback(_poll_events);
                
                register_event_handler<WindowEvent>(window_window_event_callback, TargetThread::Render);

                init_display();

                g_wm_module_initialized = true;
                
                break;
            }
            case LifecycleStage::Deinit:
                _clean_up();

                _ARGUS_DEBUG("Finished deinitializing wm");

                break;
            default:
                break;
        }
    }
}

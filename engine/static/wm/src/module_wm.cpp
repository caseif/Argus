/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/core/downstream_config.hpp"
#include "argus/core/engine.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/defines.hpp"
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

    // maps window IDs to Window instance pointers
    std::map<std::string, Window *> g_window_id_map;
    // maps GLFW window pointers to Window instance pointers
    std::map<GLFWwindow *, Window *> g_window_handle_map;
    size_t g_window_count = 0;

    static void _clean_up(void) {
        // use a copy since Window destructor modifies the global list
        auto windows_copy = g_window_id_map;
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
        Logger::default_logger().warn("GLFW Error: %s", desc);
    }

    static void _create_initial_window(void) {
        auto params = get_initial_window_parameters();
        if (!params.has_value() || !params->id.has_value() || params->id->empty()) {
            return;
        }

        auto &window = *new Window(params->id.value());

        if (params->title.has_value()) {
            window.set_title(*params->title);
        }

        if (params->mode.has_value()) {
            if (*params->mode == WINDOWING_MODE_WINDOWED) {
                window.set_fullscreen(false);
            } else if (*params->mode == WINDOWING_MODE_BORDERLESS) {
                //TODO
            } else if (*params->mode == WINDOWING_MODE_FULLSCREEN) {
                window.set_fullscreen(true);
            }
        }

        if (params->vsync.has_value()) {
            printf("found vsync value\n");
            window.set_vsync_enabled(*params->vsync);
        }

        if (params->mouse_visible.has_value()) {
            window.set_mouse_visible(*params->mouse_visible);
        }

        if (params->mouse_captured.has_value()) {
            window.set_mouse_captured(*params->mouse_captured);
        }

        if (params->mouse_raw_input.has_value()) {
            window.set_mouse_raw_input(*params->mouse_raw_input);
        }

        if (params->position.has_value()) {
            window.set_windowed_position(*params->position);
        }

        if (params->dimensions.has_value()) {
            window.set_windowed_resolution(*params->dimensions);
        }

        window.commit();
    }

    void update_lifecycle_wm(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
                int glfw_init_rc = glfwInit();
                if (glfw_init_rc != GLFW_TRUE) {
                    Logger::default_logger().fatal("GLFW init failed (return code %d)", glfw_init_rc);
                }

                Logger::default_logger().info("GLFW initialized successfully");

                glfwSetErrorCallback(_on_glfw_error);

                Logger::default_logger().debug("Initialized GLFW");

                register_render_callback(_poll_events);

                register_event_handler<WindowEvent>(window_window_event_callback, TargetThread::Render);

                init_display();

                g_wm_module_initialized = true;

                break;
            }
            case LifecycleStage::PostInit: {
                _create_initial_window();

                break;
            }
            case LifecycleStage::Deinit:
                _clean_up();

                Logger::default_logger().debug("Finished deinitializing wm");

                break;
            default:
                break;
        }
    }
}

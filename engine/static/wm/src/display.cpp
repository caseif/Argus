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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/vector.hpp"

#include "argus/wm/display.hpp"
#include "internal/wm/display.hpp"
#include "internal/wm/pimpl/display.hpp"

#include "GLFW/glfw3.h"

#include <algorithm>
#include <string>
#include <vector>

namespace argus {
    static std::vector<const Display *> g_displays;

    static void _add_display(GLFWmonitor *monitor) {
        auto &display = *new Display();

        display.pimpl->handle = monitor;

        display.pimpl->name = glfwGetMonitorName(monitor);

        glfwGetMonitorPos(monitor, &display.pimpl->position.x, &display.pimpl->position.y);

        int mode_count;
        auto *glfw_modes = glfwGetVideoModes(display.pimpl->handle, &mode_count);

        for (int i = 0; i < mode_count; i++) {
            auto glfw_mode = glfw_modes[i];

            if (glfw_mode.width <= 0 || glfw_mode.height <= 0) {
                Logger::default_logger().debug("Skipping video mode with non-positive dimensions (%d, %d)",
                        glfw_mode.width, glfw_mode.height);
                continue;
            }

            if (glfw_mode.redBits < 0 || glfw_mode.greenBits < 0 || glfw_mode.blueBits < 0) {
                Logger::default_logger().debug("Skipping video mode with negative color width (r=%d, g=%d, b=%d)",
                        glfw_mode.redBits, glfw_mode.greenBits, glfw_mode.blueBits);
                continue;
            }

            display.pimpl->modes.push_back(DisplayMode{
                    Vector2u(uint32_t(glfw_mode.width), uint32_t(glfw_mode.height)),
                    uint16_t(glfw_mode.refreshRate),
                    Vector3u(uint32_t(glfw_mode.redBits), uint32_t(glfw_mode.greenBits), uint32_t(glfw_mode.blueBits))
            });
        }

        g_displays.push_back(&display);
    }

    static void _remove_display(const GLFWmonitor *monitor) {
        auto *display = get_display_from_handle(monitor);

        if (display == nullptr) {
            return;
        }

        remove_from_vector(g_displays, display);
        delete display;
    }

    static void _enumerate_displays(void) {
        std::vector<Display> displays;

        int count;
        auto **monitors = glfwGetMonitors(&count);

        for (int i = 0; i < count; i++) {
            auto *monitor = monitors[i];

            _add_display(monitor);
        }
    }

    static void _monitor_callback(GLFWmonitor *monitor, int type) {
        if (type == GLFW_CONNECTED) {
            _add_display(monitor);
        } else if (type == GLFW_DISCONNECTED) {
            _remove_display(monitor);
        }
    }

    void init_display(void) {
        _enumerate_displays();

        glfwSetMonitorCallback(_monitor_callback);
    }

    const Display *get_display_from_handle(const GLFWmonitor *monitor) {
        auto it = std::find_if(g_displays.begin(), g_displays.end(),
                [monitor](auto *display) { return display->pimpl->handle == monitor; });
        if (it == g_displays.end()) {
            return nullptr;
        }

        return *it;
    }

    const std::vector<const Display *> &Display::get_available_displays(void) {
        return g_displays;
    }

    Display::Display(void) :
            pimpl(new pimpl_Display()) {
    }

    Display::~Display(void) {
        if (pimpl != nullptr) {
            delete pimpl;
        }
    }

    const std::string &Display::get_name(void) const {
        return pimpl->name;
    }

    Vector2i Display::get_position(void) const {
        return pimpl->position;
    }

    const std::vector<DisplayMode> &Display::get_display_modes(void) const {
        return pimpl->modes;
    }
}

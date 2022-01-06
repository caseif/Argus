/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/window.hpp"

// module input
#include "argus/input/mouse.hpp"

#include <GLFW/glfw3.h>

#include <map>
#include <mutex>

namespace argus { namespace input {
    static argus::Vector2d g_last_mouse_pos;

    static std::mutex g_window_mutex;

    static std::map<const argus::Window*, bool> g_pending_mouse_captured;
    static std::map<const argus::Window*, bool> g_pending_mouse_visible;
    static std::map<const argus::Window*, bool> g_pending_mouse_raw_input;

    argus::Vector2d mouse_position(const argus::Window &window) {
        //TODO
        UNUSED(window);
        return {};
    }

    argus::Vector2d mouse_delta(const argus::Window &window) {
        //TODO
        UNUSED(window);
        return {};
    }

    static void _set_mouse_captured_0(const argus::Window &window, bool captured) {
        auto *glfw_window = static_cast<GLFWwindow*>(get_window_handle(window));

        glfwSetInputMode(glfw_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    void set_mouse_captured(const argus::Window &window, bool captured) {
        {
            std::lock_guard<std::mutex> guard(g_window_mutex);

            if (!window.is_created()) {
                g_pending_mouse_captured[&window] = captured;
                return;
            }
        }

        _set_mouse_captured_0(window, captured);
    }
    
    static void _set_mouse_visible_0(const argus::Window &window, bool visible) {
        auto *glfw_window = static_cast<GLFWwindow*>(get_window_handle(window));
        
        auto cur_mode = glfwGetInputMode(glfw_window, GLFW_CURSOR);
        if (cur_mode == GLFW_CURSOR_DISABLED) {
            return;
        }

        glfwSetInputMode(glfw_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }

    void set_mouse_visible(const argus::Window &window, bool visible) {
        {
            std::lock_guard<std::mutex> guard(g_window_mutex);

            if (!window.is_created()) {
                g_pending_mouse_visible[&window] = visible;
                return;
            }
        }

        _set_mouse_visible_0(window, visible);
    }

    static void _set_mouse_raw_input_0(const argus::Window &window, bool raw_input) {
        if (!glfwRawMouseMotionSupported()) {
            return;
        }

        auto *glfw_window = static_cast<GLFWwindow*>(get_window_handle(window));

        glfwSetInputMode(glfw_window, GLFW_RAW_MOUSE_MOTION, raw_input ? GLFW_TRUE : GLFW_FALSE);
    }

    void set_mouse_raw_input(const argus::Window &window, bool raw_input) {
        {
            std::lock_guard<std::mutex> guard(g_window_mutex);

            if (!window.is_created()) {
                g_pending_mouse_raw_input[&window] = raw_input;
                return;
            }
        }

        _set_mouse_raw_input_0(window, raw_input);
    }

    void init_mouse(const argus::Window &window) {
        std::lock_guard<std::mutex> guard(g_window_mutex);
        
        auto it_captured = g_pending_mouse_captured.find(&window);
        if (it_captured != g_pending_mouse_captured.end()) {
            _set_mouse_captured_0(*it_captured->first, it_captured->second);
        }
        
        auto it_visible = g_pending_mouse_visible.find(&window);
        if (it_visible != g_pending_mouse_visible.end()) {
            _set_mouse_visible_0(*it_visible->first, it_visible->second);
        }
        
        auto it_raw_input = g_pending_mouse_raw_input.find(&window);
        if (it_raw_input != g_pending_mouse_raw_input.end()) {
            _set_mouse_raw_input_0(*it_raw_input->first, it_raw_input->second);
        }
    }
}}

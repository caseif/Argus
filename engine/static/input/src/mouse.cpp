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
#include "argus/lowlevel/types.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/window.hpp"

// module input
#include "argus/input/input_manager.hpp"
#include "argus/input/mouse.hpp"
#include "internal/input/mouse.hpp"

#include <GLFW/glfw3.h>

#include <map>
#include <mutex>

#include <cmath>

namespace argus { namespace input {
    static std::map<const argus::Window*, MouseState> g_mouse_states;

    argus::Vector2d mouse_position(const argus::Window &window) {
        auto *glfw_window = static_cast<GLFWwindow*>(get_window_handle(window));

        double x;
        double y;
        glfwGetCursorPos(glfw_window, &x, &y);

        return Vector2d(x, y);
    }

    argus::Vector2d mouse_delta(const argus::Window &window) {
        return g_mouse_states[&window].mouse_delta;
    }

    static void _set_mouse_captured_0(const argus::Window &window, bool captured) {
        auto *glfw_window = static_cast<GLFWwindow*>(get_window_handle(window));

        glfwSetInputMode(glfw_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    void set_mouse_captured(const argus::Window &window, bool captured) {
        auto &state = g_mouse_states[&window];
        {
            std::lock_guard<std::mutex> guard(state.window_mutex);

            if (!window.is_created()) {
                state.pending_mouse_captured = captured;
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
        auto &state = g_mouse_states[&window];

        {
            std::lock_guard<std::mutex> guard(state.window_mutex);

            if (!window.is_created()) {
                state.pending_mouse_visible = visible;
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
        auto &state = g_mouse_states[&window];

        {
            std::lock_guard<std::mutex> guard(state.window_mutex);

            if (!window.is_created()) {
                state.pending_mouse_raw_input = raw_input;
                return;
            }
        }

        _set_mouse_raw_input_0(window, raw_input);
    }

    static void _mouse_button_callback(GLFWwindow *glfw_window, int button, int action, int mods) {
        UNUSED(mods);

        if (action != GLFW_PRESS && action != GLFW_RELEASE) {
            return;
        }

        auto *window = get_window_from_handle(glfw_window);
        if (window == nullptr) {
            return;
        }

        auto release = action == GLFW_RELEASE;

        InputManager::instance().handle_mouse_button_press(*window, static_cast<MouseButton>(button), release);
    }

    static void _cursor_pos_callback(GLFWwindow *glfw_window, double x, double y) {
        auto *window = get_window_from_handle(glfw_window);
        if (window == nullptr) {
            return;
        }

        auto &state = g_mouse_states[window];

        if (state.got_first_mouse_pos) {
            auto dx = x - state.last_mouse_pos.x;
            auto dy = -(y - state.last_mouse_pos.y);

            const auto res = window->get_resolution();
            const auto min_window_dim = std::min(res.x, res.y);

            state.mouse_delta.x = dx / min_window_dim;
            state.mouse_delta.y = dy / min_window_dim;
        } else {
            state.got_first_mouse_pos = true;

            state.mouse_delta.x = 0;
            state.mouse_delta.y = 0;
        }

        state.last_mouse_pos.x = x;
        state.last_mouse_pos.y = y;

        InputManager::instance().handle_mouse_axis_change(*window, MouseAxis::Horizontal, x, state.mouse_delta.x);
        InputManager::instance().handle_mouse_axis_change(*window, MouseAxis::Vertical, y, state.mouse_delta.y);
    }

    void init_mouse(const argus::Window &window) {
        auto &state = g_mouse_states[&window];

        std::lock_guard<std::mutex> guard(state.window_mutex);

        if (state.pending_mouse_captured.is_set()) {
            _set_mouse_captured_0(window, state.pending_mouse_captured);
        }
        
        if (state.pending_mouse_visible.is_set()) {
            _set_mouse_visible_0(window, state.pending_mouse_visible);
        }
        
        if (state.pending_mouse_raw_input.is_set()) {
            _set_mouse_raw_input_0(window, state.pending_mouse_raw_input);
        }

        auto *glfw_window = static_cast<GLFWwindow*>(get_window_handle(window));

        glfwSetMouseButtonCallback(glfw_window, _mouse_button_callback);
        glfwSetCursorPosCallback(glfw_window, _cursor_pos_callback);
    }
}}

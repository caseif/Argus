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

namespace argus { namespace input {
    static argus::Vector2d g_last_mouse_pos;

    void init_mouse(const argus::Window &window) {
        auto glfw_handle = static_cast<GLFWwindow*>(argus::get_window_handle(window));
        UNUSED(glfw_handle);
        //TODO
    }

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
}}

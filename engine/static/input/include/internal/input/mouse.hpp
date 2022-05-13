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

#pragma once

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/types.hpp"

#include <GLFW/glfw3.h>

#include <mutex>

namespace argus {
    // forward declarations
    class Window;

    namespace input {
        struct MouseState {
            Vector2d last_mouse_pos;
            Vector2d mouse_delta;
            bool got_first_mouse_pos;

            std::mutex window_mutex;
        };

        void init_mouse(const argus::Window &window);
    }
}

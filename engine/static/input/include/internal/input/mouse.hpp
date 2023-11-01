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

#pragma once

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/types.hpp"

#include <mutex>

namespace argus {
    // forward declarations
    class Window;

    namespace input {
        struct MouseState {
            Vector2d last_pos;
            Vector2d delta;
            bool got_first_pos = false;
            bool is_delta_stale = false;
            uint32_t button_state = 0;
        };

        void init_mouse(const argus::Window &window);

        void update_mouse(void);

        void flush_mouse_delta(void);
    }
}

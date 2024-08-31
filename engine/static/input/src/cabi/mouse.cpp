/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/input/cabi/mouse.h"

#include "argus/input/mouse.hpp"

#include "argus/core/engine.hpp"

extern"C" {

argus_vector_2d_t argus_mouse_delta(void) {
    auto delta = argus::input::mouse_delta();
    return *reinterpret_cast<argus_vector_2d_t *>(&delta);
}

argus_vector_2d_t argus_mouse_pos(void) {
    auto pos = argus::input::mouse_pos();
    return *reinterpret_cast<argus_vector_2d_t *>(&pos);
}

double argus_get_mouse_axis(ArgusMouseAxis axis) {
    return argus::input::get_mouse_axis(argus::input::MouseAxis(axis));
}

double argus_get_mouse_axis_delta(ArgusMouseAxis axis) {
    return argus::input::get_mouse_axis_delta(argus::input::MouseAxis(axis));
}

bool argus_is_mouse_button_pressed(ArgusMouseButton button) {
    return argus::input::is_mouse_button_pressed(argus::input::MouseButton(button));
}

}

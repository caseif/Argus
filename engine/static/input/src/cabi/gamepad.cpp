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

#include "argus/input/cabi/gamepad.h"

#include "argus/input/gamepad.hpp"

#include <stdint.h>

extern"C" {

uint8_t argus_get_connected_gamepad_count(void) {
    return argus::input::get_connected_gamepad_count();
}

uint8_t argus_get_unattached_gamepad_count(void) {
    return argus::input::get_unattached_gamepad_count();
}

const char *argus_get_gamepad_name(ArgusHidDeviceId gamepad) {
    return argus::input::get_gamepad_name(gamepad);
}

bool argus_is_gamepad_button_pressed(ArgusHidDeviceId gamepad, ArgusGamepadButton button) {
    return argus::input::is_gamepad_button_pressed(gamepad, argus::input::GamepadButton(button));
}

double argus_get_gamepad_axis(ArgusHidDeviceId gamepad, ArgusGamepadAxis axis) {
    return argus::input::get_gamepad_axis(gamepad, argus::input::GamepadAxis(axis));
}

double argus_get_gamepad_axis_delta(ArgusHidDeviceId gamepad, ArgusGamepadAxis axis) {
    return argus::input::get_gamepad_axis_delta(gamepad, argus::input::GamepadAxis(axis));
}

}

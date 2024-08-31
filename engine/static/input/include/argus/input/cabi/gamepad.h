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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef int ArgusHidDeviceId;

typedef enum ArgusGamepadButton {
    GAMEPAD_BUTTON_UNKNOWN = -1,
    GAMEPAD_BUTTON_A,
    GAMEPAD_BUTTON_B,
    GAMEPAD_BUTTON_X,
    GAMEPAD_BUTTON_Y,
    GAMEPAD_BUTTON_DPAD_UP,
    GAMEPAD_BUTTON_DPAD_DOWN,
    GAMEPAD_BUTTON_DPAD_LEFT,
    GAMEPAD_BUTTON_DPAD_RIGHT,
    GAMEPAD_BUTTON_L_BUMPER,
    GAMEPAD_BUTTON_R_BUMPER,
    GAMEPAD_BUTTON_L_TRIGGER,
    GAMEPAD_BUTTON_R_TRIGGER,
    GAMEPAD_BUTTON_L_STICK,
    GAMEPAD_BUTTON_R_STICK,
    GAMEPAD_BUTTON_L4,
    GAMEPAD_BUTTON_R4,
    GAMEPAD_BUTTON_L5,
    GAMEPAD_BUTTON_R5,
    GAMEPAD_BUTTON_START,
    GAMEPAD_BUTTON_BACK,
    GAMEPAD_BUTTON_GUIDE,
    GAMEPAD_BUTTON_MISC_1,
    GAMEPAD_BUTTON_MAX_VALUE,
} ArgusGamepadButton;

typedef enum ArgusGamepadAxis {
    GAMEPAD_AXIS_UNKNOWN = -1,
    GAMEPAD_AXIS_LEFT_X,
    GAMEPAD_AXIS_LEFT_Y,
    GAMEPAD_AXIS_RIGHT_X,
    GAMEPAD_AXIS_RIGHT_Y,
    GAMEPAD_AXIS_L_TRIGGER,
    GAMEPAD_AXIS_R_TRIGGER,
    GAMEPAD_AXIS_MAX_VALUE,
} ArgusGamepadAxis;

uint8_t argus_get_connected_gamepad_count(void);

uint8_t argus_get_unattached_gamepad_count(void);

const char *argus_get_gamepad_name(ArgusHidDeviceId gamepad);

bool argus_is_gamepad_button_pressed(ArgusHidDeviceId gamepad, ArgusGamepadButton button);

double argus_get_gamepad_axis(ArgusHidDeviceId gamepad, ArgusGamepadAxis axis);

double argus_get_gamepad_axis_delta(ArgusHidDeviceId gamepad, ArgusGamepadAxis axis);

#ifdef __cplusplus
}
#endif

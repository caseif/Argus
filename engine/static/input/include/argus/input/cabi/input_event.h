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

#include "argus/input/cabi/gamepad.h"

#include "argus/wm/cabi/window.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *const k_event_type_input = "input";

static const char *const k_event_type_input_device = "input_device";

typedef void *argus_input_event_t;
typedef const void *argus_input_event_const_t;

typedef void *argus_input_device_event_t;
typedef const void *argus_input_device_event_const_t;

typedef enum ArgusInputEventType {
    INPUT_EVENT_TYPE_BUTTON_DOWN,
    INPUT_EVENT_TYPE_BUTTON_UP,
    INPUT_EVENT_TYPE_AXIS_CHANGED,
} ArgusInputEventType;

typedef enum ArgusInputDeviceEventType {
    INPUT_DEV_EVENT_TYPE_GAMEPAD_CONNECTED,
    INPUT_DEV_EVENT_TYPE_GAMEPAD_DISCONNECTED,
} ArgusInputDeviceEventType;

ArgusInputEventType argus_input_event_get_input_type(argus_input_event_const_t event);

argus_window_const_t argus_input_event_get_window(argus_input_event_const_t event);

const char *argus_input_event_get_controller_name(argus_input_event_const_t event);

const char *argus_input_event_get_action(argus_input_event_const_t event);

double argus_input_event_get_axis_value(argus_input_event_const_t event);

double argus_input_event_get_axis_delta(argus_input_event_const_t event);

ArgusInputDeviceEventType argus_input_device_event_get_device_event(argus_input_device_event_const_t event);

const char *argus_input_device_event_get_controller_name(argus_input_device_event_const_t event);

ArgusHidDeviceId argus_input_device_event_get_device_id(argus_input_device_event_const_t event);

#ifdef __cplusplus
}
#endif

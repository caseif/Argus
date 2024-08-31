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

#include "argus/input/cabi/input_event.h"

#include "argus/input/input_event.hpp"

using namespace argus::input;

static const InputEvent &_event_as_ref(argus_input_event_const_t event) {
    return *reinterpret_cast<const InputEvent *>(event);
}

static const InputDeviceEvent &_dev_event_as_ref(argus_input_device_event_const_t event) {
    return *reinterpret_cast<const InputDeviceEvent *>(event);
}

extern"C" {

ArgusInputEventType argus_input_event_get_input_type(argus_input_event_const_t event) {
    return ArgusInputEventType(_event_as_ref(event).input_type);
}

argus_window_const_t argus_input_event_get_window(argus_input_event_const_t event) {
    return _event_as_ref(event).window;
}

const char *argus_input_event_get_controller_name(argus_input_event_const_t event) {
    return _event_as_ref(event).controller_name.c_str();
}

const char *argus_input_event_get_action(argus_input_event_const_t event) {
    return _event_as_ref(event).action.c_str();
}

double argus_input_event_get_axis_value(argus_input_event_const_t event) {
    return _event_as_ref(event).axis_value;
}

double argus_input_event_get_axis_delta(argus_input_event_const_t event) {
    return _event_as_ref(event).axis_delta;
}

ArgusInputDeviceEventType argus_input_device_event_get_device_event(argus_input_device_event_const_t event) {
    return ArgusInputDeviceEventType(_dev_event_as_ref(event).device_event);
}

const char *argus_input_device_event_get_controller_name(argus_input_device_event_const_t event) {
    return _dev_event_as_ref(event).controller_name.c_str();
}

ArgusHidDeviceId argus_input_device_event_get_device_id(argus_input_device_event_const_t event) {
    return _dev_event_as_ref(event).device_id;
}

}

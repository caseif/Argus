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

#include "keyboard.h"
#include "gamepad.h"
#include "mouse.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *argus_controller_t;
typedef const void *argus_controller_const_t;

typedef enum ArgusDeadzoneShape {
    DEADZONE_SHAPE_ELLIPSE,
    DEADZONE_SHAPE_QUAD,
    DEADZONE_SHAPE_CROSS,
    DEADZONE_SHAPE_MAXVALUE,
} ArgusDeadzoneShape;

const char *argus_controller_get_name(argus_controller_const_t controller);

bool argus_controller_has_gamepad(argus_controller_const_t controller);

void argus_controller_attach_gamepad(argus_controller_t controller, ArgusHidDeviceId id);

bool argus_controller_attach_first_available_gamepad(argus_controller_t controller);

void argus_controller_detach_gamepad(argus_controller_t controller);

const char *argus_controller_get_gamepad_name(argus_controller_const_t controller);

double argus_controller_get_deadzone_radius(argus_controller_const_t controller);

void argus_controller_set_deadzone_radius(argus_controller_t controller, double radius);

void argus_controller_clear_deadzone_radius(argus_controller_t controller);

ArgusDeadzoneShape argus_controller_get_deadzone_shape(argus_controller_const_t controller);

void argus_controller_set_deadzone_shape(argus_controller_t controller, ArgusDeadzoneShape shape);

void argus_controller_clear_deadzone_shape(argus_controller_t controller);

double argus_controller_get_axis_deadzone_radius(argus_controller_const_t controller, ArgusGamepadAxis axis);

void argus_controller_set_axis_deadzone_radius(argus_controller_t controller, ArgusGamepadAxis axis, double radius);

void argus_controller_clear_axis_deadzone_radius(argus_controller_t controller, ArgusGamepadAxis axis);

ArgusDeadzoneShape argus_controller_get_axis_deadzone_shape(argus_controller_const_t controller, ArgusGamepadAxis axis);

void argus_controller_set_axis_deadzone_shape(argus_controller_t controller, ArgusGamepadAxis axis, ArgusDeadzoneShape shape);

void argus_controller_clear_axis_deadzone_shape(argus_controller_t controller, ArgusGamepadAxis axis);

void argus_controller_unbind_action(argus_controller_t controller, const char *action);

size_t argus_controller_get_keyboard_key_bindings_count(argus_controller_const_t controller, ArgusKeyboardScancode key);

void argus_controller_get_keyboard_key_bindings(argus_controller_const_t controller, ArgusKeyboardScancode key,
        const char **out_bindings, size_t count);

size_t argus_controller_get_keyboard_action_bindings_count(argus_controller_const_t controller, const char *action);

void argus_controller_get_keyboard_action_bindings(argus_controller_const_t controller, const char *action,
        ArgusKeyboardScancode *out_scancodes, size_t count);

void argus_controller_bind_keyboard_key(argus_controller_t controller, ArgusKeyboardScancode key, const char *action);

void argus_controller_unbind_keyboard_key(argus_controller_t controller, ArgusKeyboardScancode key);

void argus_controller_unbind_keyboard_key_action(argus_controller_t controller, ArgusKeyboardScancode key, const char *action);

void argus_controller_bind_mouse_button(argus_controller_t controller, ArgusMouseButton button, const char *action);

void argus_controller_unbind_mouse_button(argus_controller_t controller, ArgusMouseButton button);

void argus_controller_unbind_mouse_button_action(argus_controller_t controller, ArgusMouseButton button, const char *action);

void argus_controller_bind_mouse_axis(argus_controller_t controller, ArgusMouseAxis axis, const char *action);

void argus_controller_unbind_mouse_axis(argus_controller_t controller, ArgusMouseAxis axis);

void argus_controller_unbind_mouse_axis_action(argus_controller_t controller, ArgusMouseAxis axis, const char *action);

void argus_controller_bind_gamepad_button(argus_controller_t controller, ArgusGamepadButton button, const char *action);

void argus_controller_unbind_gamepad_button(argus_controller_t controller, ArgusGamepadButton button);

void argus_controller_unbind_gamepad_button_action(argus_controller_t controller, ArgusGamepadButton button, const char *action);

void argus_controller_bind_gamepad_axis(argus_controller_t controller, ArgusGamepadAxis axis, const char *action);

void argus_controller_unbind_gamepad_axis(argus_controller_t controller, ArgusGamepadAxis axis);

void argus_controller_unbind_gamepad_axis_action(argus_controller_t controller, ArgusGamepadAxis axis, const char *action);

bool argus_controller_is_gamepad_button_pressed(argus_controller_const_t controller, ArgusGamepadButton button);

double argus_controller_get_gamepad_axis(argus_controller_const_t controller, ArgusGamepadAxis axis);

double argus_controller_get_gamepad_axis_delta(argus_controller_const_t controller, ArgusGamepadAxis axis);

bool argus_controller_is_action_pressed(argus_controller_const_t controller, const char *action);

double argus_controller_get_action_axis(argus_controller_const_t controller, const char *action);

double argus_controller_get_action_axis_delta(argus_controller_const_t controller, const char *action);

#ifdef __cplusplus
}
#endif

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

#include <algorithm>
#include "argus/input/cabi/controller.h"
#include "argus/input/controller.hpp"

using argus::input::Controller;

static Controller &_as_ref(argus_controller_t controller) {
    return *reinterpret_cast<Controller *>(controller);
}

static const Controller &_as_ref(argus_controller_const_t controller) {
    return *reinterpret_cast<const Controller *>(controller);
}

extern"C" {

const char *argus_controller_get_name(argus_controller_const_t controller) {
    return _as_ref(controller).get_name().c_str();
}

bool argus_controller_has_gamepad(argus_controller_const_t controller) {
    return _as_ref(controller).has_gamepad();
}

void argus_controller_attach_gamepad(argus_controller_t controller, ArgusHidDeviceId id) {
    _as_ref(controller).attach_gamepad(id);
}

bool argus_controller_attach_first_available_gamepad(argus_controller_t controller) {
    return _as_ref(controller).attach_first_available_gamepad();
}

void argus_controller_detach_gamepad(argus_controller_t controller) {
    _as_ref(controller).detach_gamepad();
}

const char *argus_controller_get_gamepad_name(argus_controller_const_t controller) {
    return _as_ref(controller).get_gamepad_name();
}

double argus_controller_get_deadzone_radius(argus_controller_const_t controller) {
    return _as_ref(controller).get_deadzone_radius();
}

void argus_controller_set_deadzone_radius(argus_controller_t controller, double radius) {
    _as_ref(controller).set_deadzone_radius(radius);
}

void argus_controller_clear_deadzone_radius(argus_controller_t controller) {
    _as_ref(controller).clear_deadzone_radius();
}

ArgusDeadzoneShape argus_controller_get_deadzone_shape(argus_controller_const_t controller) {
    return ArgusDeadzoneShape(_as_ref(controller).get_deadzone_shape());
}

void argus_controller_set_deadzone_shape(argus_controller_t controller, ArgusDeadzoneShape shape) {
    _as_ref(controller).set_deadzone_shape(argus::input::DeadzoneShape(shape));
}

void argus_controller_clear_deadzone_shape(argus_controller_t controller) {
    _as_ref(controller).clear_deadzone_shape();
}

double argus_controller_get_axis_deadzone_radius(argus_controller_const_t controller, ArgusGamepadAxis axis) {
    return _as_ref(controller).get_axis_deadzone_radius(argus::input::GamepadAxis(axis));
}

void argus_controller_set_axis_deadzone_radius(argus_controller_t controller, ArgusGamepadAxis axis, double radius) {
    _as_ref(controller).set_axis_deadzone_radius(argus::input::GamepadAxis(axis), radius);
}

void argus_controller_clear_axis_deadzone_radius(argus_controller_t controller, ArgusGamepadAxis axis) {
    _as_ref(controller).clear_axis_deadzone_radius(argus::input::GamepadAxis(axis));
}

ArgusDeadzoneShape argus_controller_get_axis_deadzone_shape(argus_controller_const_t controller,
        ArgusGamepadAxis axis) {
    return ArgusDeadzoneShape(_as_ref(controller).get_axis_deadzone_shape(argus::input::GamepadAxis(axis)));
}

void argus_controller_set_axis_deadzone_shape(argus_controller_t controller, ArgusGamepadAxis axis,
        ArgusDeadzoneShape shape) {
    _as_ref(controller).set_axis_deadzone_shape(argus::input::GamepadAxis(axis), argus::input::DeadzoneShape(shape));
}

void argus_controller_clear_axis_deadzone_shape(argus_controller_t controller, ArgusGamepadAxis axis) {
    _as_ref(controller).clear_axis_deadzone_shape(argus::input::GamepadAxis(axis));
}

void argus_controller_unbind_action(argus_controller_t controller, const char *action) {
    _as_ref(controller).unbind_action(action);
}

size_t argus_controller_get_keyboard_key_bindings_count(argus_controller_const_t controller,
        ArgusKeyboardScancode key) {
    return _as_ref(controller).get_keyboard_key_bindings(argus::input::KeyboardScancode(key)).size();
}

void argus_controller_get_keyboard_key_bindings(argus_controller_const_t controller, ArgusKeyboardScancode key,
        const char **out_bindings, size_t count) {
    const auto bindings = _as_ref(controller).get_keyboard_key_bindings(argus::input::KeyboardScancode(key));
    affirm_precond(count == bindings.size(),
            "argus_controller_get_keyboard_key_bindings called with wrong count parameter");
    std::transform(bindings.cbegin(), bindings.cend(), out_bindings,
            [](auto action_ref) { return action_ref.get().c_str(); });
}

size_t argus_controller_get_keyboard_action_bindings_count(argus_controller_const_t controller, const char *action) {
    return _as_ref(controller).get_keyboard_action_bindings(action).size();
}

void argus_controller_get_keyboard_action_bindings(argus_controller_const_t controller, const char *action,
        ArgusKeyboardScancode *out_scancodes, size_t count) {
    const auto bindings = _as_ref(controller).get_keyboard_action_bindings(action);
    affirm_precond(count == bindings.size(),
            "argus_controller_get_keyboard_action_bindings called with wrong count parameter");
    std::transform(bindings.cbegin(), bindings.cend(), out_scancodes,
            [](auto key) { return ArgusKeyboardScancode(key); });
}

void argus_controller_bind_keyboard_key(argus_controller_t controller, ArgusKeyboardScancode key, const char *action) {
    _as_ref(controller).bind_keyboard_key(argus::input::KeyboardScancode(key), action);
}

void argus_controller_unbind_keyboard_key(argus_controller_t controller, ArgusKeyboardScancode key) {
    _as_ref(controller).unbind_keyboard_key(argus::input::KeyboardScancode(key));
}

void argus_controller_unbind_keyboard_key_action(argus_controller_t controller, ArgusKeyboardScancode key,
        const char *action) {
    _as_ref(controller).unbind_keyboard_key(argus::input::KeyboardScancode(key), action);
}

void argus_controller_bind_mouse_button(argus_controller_t controller, ArgusMouseButton button, const char *action) {
    _as_ref(controller).bind_mouse_button(argus::input::MouseButton(button), action);
}

void argus_controller_unbind_mouse_button(argus_controller_t controller, ArgusMouseButton button) {
    _as_ref(controller).unbind_mouse_button(argus::input::MouseButton(button));
}

void argus_controller_unbind_mouse_button_action(argus_controller_t controller, ArgusMouseButton button,
        const char *action) {
    _as_ref(controller).unbind_mouse_button(argus::input::MouseButton(button), action);
}

void argus_controller_bind_mouse_axis(argus_controller_t controller, ArgusMouseAxis axis, const char *action) {
    _as_ref(controller).bind_mouse_axis(argus::input::MouseAxis(axis), action);
}

void argus_controller_unbind_mouse_axis(argus_controller_t controller, ArgusMouseAxis axis) {
    _as_ref(controller).unbind_mouse_axis(argus::input::MouseAxis(axis));
}

void argus_controller_unbind_mouse_axis_action(argus_controller_t controller, ArgusMouseAxis axis, const char *action) {
    _as_ref(controller).unbind_mouse_axis(argus::input::MouseAxis(axis), action);
}

void argus_controller_bind_gamepad_button(argus_controller_t controller, ArgusGamepadButton button,
        const char *action) {
    _as_ref(controller).bind_gamepad_button(argus::input::GamepadButton(button), action);
}

void argus_controller_unbind_gamepad_button(argus_controller_t controller, ArgusGamepadButton button) {
    _as_ref(controller).unbind_gamepad_button(argus::input::GamepadButton(button));
}

void argus_controller_unbind_gamepad_button_action(argus_controller_t controller, ArgusGamepadButton button,
        const char *action) {
    _as_ref(controller).unbind_gamepad_button(argus::input::GamepadButton(button), action);
}

void argus_controller_bind_gamepad_axis(argus_controller_t controller, ArgusGamepadAxis axis, const char *action) {
    _as_ref(controller).bind_gamepad_axis(argus::input::GamepadAxis(axis), action);
}

void argus_controller_unbind_gamepad_axis(argus_controller_t controller, ArgusGamepadAxis axis) {
    _as_ref(controller).unbind_gamepad_axis(argus::input::GamepadAxis(axis));
}

void argus_controller_unbind_gamepad_axis_action(argus_controller_t controller, ArgusGamepadAxis axis,
        const char *action) {
    _as_ref(controller).unbind_gamepad_axis(argus::input::GamepadAxis(axis), action);
}

bool argus_controller_is_gamepad_button_pressed(argus_controller_const_t controller, ArgusGamepadButton button) {
    return _as_ref(controller).is_gamepad_button_pressed(argus::input::GamepadButton(button));
}

double argus_controller_get_gamepad_axis(argus_controller_const_t controller, ArgusGamepadAxis axis) {
    return _as_ref(controller).get_gamepad_axis(argus::input::GamepadAxis(axis));
}

double argus_controller_get_gamepad_axis_delta(argus_controller_const_t controller, ArgusGamepadAxis axis) {
    return _as_ref(controller).get_gamepad_axis_delta(argus::input::GamepadAxis(axis));
}

bool argus_controller_is_action_pressed(argus_controller_const_t controller, const char *action) {
    return _as_ref(controller).is_action_pressed(action);
}

double argus_controller_get_action_axis(argus_controller_const_t controller, const char *action) {
    return _as_ref(controller).get_action_axis(action);
}

double argus_controller_get_action_axis_delta(argus_controller_const_t controller, const char *action) {
    return _as_ref(controller).get_action_axis_delta(action);
}

}

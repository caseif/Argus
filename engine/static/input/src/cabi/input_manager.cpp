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

#include "argus/input/cabi/input_manager.h"

#include "argus/input/input_manager.hpp"

using argus::input::InputManager;

static InputManager &_as_ref(argus_input_manager_t manager) {
    return *reinterpret_cast<InputManager *>(manager);
}

static const InputManager &_as_ref(argus_input_manager_const_t manager) {
    return *reinterpret_cast<const InputManager *>(manager);
}

extern "C" {

argus_input_manager_t argus_input_manager_get_instance(void) {
    return &InputManager::instance();
}

argus_controller_t argus_input_manager_get_controller(argus_input_manager_t manager, const char *name) {
    return &_as_ref(manager).get_controller(name);
}

argus_controller_t argus_input_manager_add_controller(argus_input_manager_t manager, const char *name) {
    return &_as_ref(manager).add_controller(name);
}

void argus_input_manager_remove_controller(argus_input_manager_t manager, const char *name) {
    _as_ref(manager).remove_controller(name);
}

double argus_input_manager_get_global_deadzone_radius(argus_input_manager_const_t manager) {
    return _as_ref(manager).get_global_deadzone_radius();
}

void argus_input_manager_set_global_deadzone_radius(argus_input_manager_t manager, double radius) {
    _as_ref(manager).set_global_deadzone_radius(radius);
}

ArgusDeadzoneShape argus_input_manager_get_global_deadzone_shape(argus_input_manager_const_t manager) {
    return ArgusDeadzoneShape(_as_ref(manager).get_global_deadzone_shape());
}

void argus_input_manager_set_global_deadzone_shape(argus_input_manager_t manager, ArgusDeadzoneShape shape) {
    _as_ref(manager).set_global_deadzone_shape(argus::input::DeadzoneShape(shape));
}

double argus_input_manager_get_global_axis_deadzone_radius(argus_input_manager_const_t manager, ArgusGamepadAxis axis) {
    return _as_ref(manager).get_global_axis_deadzone_radius(argus::input::GamepadAxis(axis));
}

void argus_input_manager_set_global_axis_deadzone_radius(argus_input_manager_t manager, ArgusGamepadAxis axis,
        double radius) {
    _as_ref(manager).set_global_axis_deadzone_radius(argus::input::GamepadAxis(axis), radius);
}

void argus_input_manager_clear_global_axis_deadzone_radius(argus_input_manager_t manager, ArgusGamepadAxis axis) {
    _as_ref(manager).clear_global_axis_deadzone_radius(argus::input::GamepadAxis(axis));
}

ArgusDeadzoneShape argus_input_manager_get_global_axis_deadzone_shape(argus_input_manager_const_t manager,
        ArgusGamepadAxis axis) {
    return ArgusDeadzoneShape(_as_ref(manager).get_global_axis_deadzone_shape(argus::input::GamepadAxis(axis)));
}

void argus_input_manager_set_global_axis_deadzone_shape(argus_input_manager_t manager, ArgusGamepadAxis axis,
        ArgusDeadzoneShape shape) {
    _as_ref(manager).set_global_axis_deadzone_shape(argus::input::GamepadAxis(axis),
            argus::input::DeadzoneShape(shape));
}

void argus_input_manager_clear_global_axis_deadzone_shape(argus_input_manager_t manager, ArgusGamepadAxis axis) {
    _as_ref(manager).clear_global_axis_deadzone_shape(argus::input::GamepadAxis(axis));
}

}

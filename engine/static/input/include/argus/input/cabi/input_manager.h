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

#include "argus/input/cabi/controller.h"
#include "argus/input/cabi/gamepad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *argus_input_manager_t;
typedef const void *argus_input_manager_const_t;

argus_input_manager_t argus_input_manager_get_instance(void);

argus_controller_t argus_input_manager_get_controller(argus_input_manager_t manager, const char *name);

argus_controller_t argus_input_manager_add_controller(argus_input_manager_t manager, const char *name);

void argus_input_manager_remove_controller(argus_input_manager_t manager, const char *name);

double argus_input_manager_get_global_deadzone_radius(argus_input_manager_const_t manager);

void argus_input_manager_set_global_deadzone_radius(argus_input_manager_t manager, double radius);

ArgusDeadzoneShape argus_input_manager_get_global_deadzone_shape(argus_input_manager_const_t manager);

void argus_input_manager_set_global_deadzone_shape(argus_input_manager_t manager, ArgusDeadzoneShape shape);

double argus_input_manager_get_global_axis_deadzone_radius(argus_input_manager_const_t manager, ArgusGamepadAxis axis);

void argus_input_manager_set_global_axis_deadzone_radius(argus_input_manager_t manager, ArgusGamepadAxis axis,
        double radius);

void argus_input_manager_clear_global_axis_deadzone_radius(argus_input_manager_t manager, ArgusGamepadAxis axis);

ArgusDeadzoneShape argus_input_manager_get_global_axis_deadzone_shape(argus_input_manager_const_t manager,
        ArgusGamepadAxis axis);

void argus_input_manager_set_global_axis_deadzone_shape(argus_input_manager_t manager, ArgusGamepadAxis axis,
        ArgusDeadzoneShape shape);

void argus_input_manager_clear_global_axis_deadzone_shape(argus_input_manager_t manager, ArgusGamepadAxis axis);

#ifdef __cplusplus
}
#endif

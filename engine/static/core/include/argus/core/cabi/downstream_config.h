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

#include "argus/lowlevel/cabi/math/vector.h"

#include <stdbool.h>

typedef struct argus_scripting_parameters_t {
    bool has_main;
    const char *main;
} argus_scripting_parameters_t;

typedef struct argus_initial_window_parameters_t {
    bool has_id;
    const char *id;
    bool has_title;
    const char *title;
    bool has_mode;
    const char *mode;
    bool has_vsync;
    bool vsync;
    bool has_mouse_visible;
    bool mouse_visible;
    bool has_mouse_captured;
    bool mouse_captured;
    bool has_mouse_raw_input;
    bool mouse_raw_input;
    bool has_position;
    argus_vector_2i_t position;
    bool has_dimensions;
    argus_vector_2u_t dimensions;
} argus_initial_window_parameters_t;

argus_scripting_parameters_t argus_get_scripting_parameters(void);

void argus_set_scripting_parameters(const argus_scripting_parameters_t *params);

argus_initial_window_parameters_t argus_get_initial_window_parameters(void);

void argus_set_initial_window_parameters(const argus_initial_window_parameters_t *params);

const char *argus_get_default_bindings_resource_id(void);

void argus_set_default_bindings_resource_id(const char *resource_id);

bool argus_get_save_user_bindings(void);

void argus_set_save_user_bindings(bool save);

#ifdef __cplusplus
}
#endif

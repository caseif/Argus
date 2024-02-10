/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "stdbool.h"

//TODO: move to lowlevel_cabi
typedef enum TriState {
    TRISTATE_UNDEF,
    TRISTATE_FALSE,
    TRISTATE_TRUE
} TriState;

const char *get_main_script(void);

void set_main_script(const char *script_uid);

const char *get_initial_window_id(void);

void set_initial_window_id(const char *id);

const char *get_initial_window_title(void);

void set_initial_window_title(const char *title);

const char *get_initial_window_mode(void);

void set_initial_window_mode(const char *mode);

TriState get_initial_window_vsync(void);

void set_initial_window_vsync(bool vsync);

TriState get_initial_window_mouse_visible(void);

void set_initial_window_mouse_visible(bool visible);

TriState get_initial_window_mouse_captured(void);

void set_initial_window_mouse_captured(bool captured);

TriState get_initial_window_mouse_raw_input(void);

void set_initial_window_mouse_raw_input(bool raw_input);

//TODO: get_initial_window_position

//TODO: set_initial_window_position

//TODO: get_initial_window_dimensions

//TODO: set_initial_window_dimensions

const char *get_default_bindings_resource_id(void);

void set_default_bindings_resource_id(const char *resource_id);

bool get_save_user_bindings(void);

void set_save_user_bindings(bool save);

#ifdef __cplusplus
}
#endif

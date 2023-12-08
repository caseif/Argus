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

extern "C" {

#include "argus/core_cabi/screen_space.h"

#include <stddef.h>

void set_target_tickrate(unsigned int target_tickrate);

void set_target_framerate(unsigned int target_framerate);

void set_load_modules(const char **module_names, size_t count);

void add_load_module(const char *module_name);

void get_preferred_render_backends(size_t *out_count, const char **out_names);

void set_render_backends(const char **names, size_t count);

void add_render_backend(const char *name);

void set_render_backend(const char *name);

ScreenSpaceScaleMode get_screen_space_scale_mode(void);

void set_screen_space_scale_mode(ScreenSpaceScaleMode mode);

}

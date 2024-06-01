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

#include "argus/wm/cabi/window.h"

#include <stdbool.h>

typedef void *gl_context_t;

typedef enum GLContextFlags {
    GL_CONTEXT_FLAG_NONE = 0x0,
    GL_CONTEXT_FLAG_PROFILE_CORE = 0x1,
    GL_CONTEXT_FLAG_PROFILE_ES = 0x2,
    GL_CONTEXT_FLAG_PROFILE_COMPAT = 0x4,
    GL_CONTEXT_FLAG_DEBUG_CONTEXT = 0x100,
    GL_CONTEXT_FLAG_PROFILE_MASK = GL_CONTEXT_FLAG_PROFILE_CORE
            | GL_CONTEXT_FLAG_PROFILE_ES
            | GL_CONTEXT_FLAG_PROFILE_COMPAT,
} GLContextFlags;

int32_t argus_gl_load_library(void);

void argus_gl_unload_library(void);

gl_context_t argus_gl_create_context(argus_window_t window, int32_t version_major, int32_t version_minor,
        GLContextFlags flags);

void argus_gl_destroy_context(gl_context_t context);

bool argus_gl_is_context_current(gl_context_t context);

int argus_gl_make_context_current(argus_window_t window, gl_context_t context);

void *argus_gl_load_proc(const char *name);

void argus_gl_swap_interval(int32_t interval);

void argus_gl_swap_buffers(argus_window_t window);

bool argus_vk_is_supported(void);

int argus_vk_create_surface(argus_window_t window, void *instance, void **out_surface);

int argus_vk_get_required_instance_extensions(argus_window_t window, unsigned int *out_count, const char **out_names);

#ifdef __cplusplus
}
#endif

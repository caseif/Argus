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

#include "argus/wm/api_util.hpp"
#include "argus/wm/cabi/api_util.h"

using argus::Window;

#ifdef __cplusplus
extern "C" {
#endif

int argus_gl_load_library(void) {
    return argus::gl_load_library();
}

void argus_gl_unload_library(void) {
    argus::gl_unload_library();
}

gl_context_t argus_gl_create_context(argus_window_t window, int version_major, int version_minor,
        GLContextFlags flags) {
    return argus::gl_create_context(*reinterpret_cast<Window *>(window), version_major, version_minor,
            argus::GLContextFlags(flags));
}

void argus_gl_destroy_context(gl_context_t context) {
    argus::gl_destroy_context(context);
}

bool argus_gl_is_context_current(gl_context_t context) {
    return argus::gl_is_context_current(context);
}

int argus_gl_make_context_current(argus_window_t window, gl_context_t context) {
    return argus::gl_make_context_current(*reinterpret_cast<Window *>(window), context);
}

void *argus_gl_load_proc(const char *name) {
    return argus::gl_load_proc(name);
}

void argus_gl_swap_interval(int interval) {
    return argus::gl_swap_interval(interval);
}

void argus_gl_swap_buffers(argus_window_t window) {
    argus::gl_swap_buffers(*reinterpret_cast<Window *>(window));
}

bool argus_vk_is_supported(void) {
    return argus::vk_is_supported();
}

int argus_vk_create_surface(argus_window_t window, void *instance, void **out_surface) {
    return argus::vk_create_surface(*reinterpret_cast<Window *>(window), instance, out_surface);
}

int argus_vk_get_required_instance_extensions(argus_window_t window, unsigned int *out_count, const char **out_names) {
    return argus::vk_get_required_instance_extensions(*reinterpret_cast<Window *>(window), out_count, out_names);
}

#ifdef __cplusplus
}
#endif

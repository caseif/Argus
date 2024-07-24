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

#include "argus/lowlevel/cabi/math.h"

#include "argus/wm/cabi/display.h"

#include <stdbool.h>
#include <stdint.h>

typedef void *argus_window_t;
typedef const void *argus_window_const_t;
typedef void *argus_canvas_t;
typedef const void *argus_canvas_const_t;

typedef void (*argus_window_callback_t)(argus_window_t);

typedef argus_canvas_t (*argus_canvas_ctor_t)(argus_window_t);

typedef void (*argus_canvas_dtor_t)(argus_canvas_t);

typedef enum WindowCreateFlags {
    WINDOW_CREATE_FLAG_NONE                = 0x0,
    WINDOW_CREATE_FLAG_OPENGL              = 0x1,
    WINDOW_CREATE_FLAG_VULKAN              = 0x2,
    WINDOW_CREATE_FLAG_METAL               = 0x4,
    WINDOW_CREATE_FLAG_DIRECTX             = 0x8,
    WINDOW_CREATE_FLAG_WEBGPU              = 0x10,

    WINDOW_CREATE_FLAG_GRAPHICS_API_MASK = WINDOW_CREATE_FLAG_OPENGL
            | WINDOW_CREATE_FLAG_VULKAN
            | WINDOW_CREATE_FLAG_METAL
            | WINDOW_CREATE_FLAG_DIRECTX
            | WINDOW_CREATE_FLAG_WEBGPU,
} WindowCreateFlags;

void argus_set_window_creation_flags(WindowCreateFlags flags);

argus_window_t argus_get_window(const char *id);

void *argus_get_window_handle(argus_window_const_t window);

argus_window_t argus_get_window_from_handle(const void *handle);

void argus_window_set_canvas_ctor_and_dtor(argus_canvas_ctor_t ctor, argus_canvas_dtor_t dtor);

argus_window_t argus_window_create(const char *id, argus_window_t parent);

const char *argus_window_get_id(argus_window_const_t self);

argus_canvas_t argus_window_get_canvas(argus_window_const_t self);

bool argus_window_is_created(argus_window_const_t self);

bool argus_window_is_ready(argus_window_const_t self);

bool argus_window_is_close_request_pending(argus_window_const_t self);

bool argus_window_is_closed(argus_window_const_t self);

argus_window_t argus_window_create_child_window(argus_window_t self, const char *id);

void argus_window_remove_child(argus_window_t self, argus_window_const_t child);

void argus_window_update(argus_window_t self, uint64_t delta_us);

void argus_window_set_title(argus_window_t self, const char *title);

bool argus_window_is_fullscreen(argus_window_const_t self);

void argus_window_set_fullscreen(argus_window_t self, bool fullscreen);

void argus_window_get_resolution(argus_window_t self, argus_vector_2u_t *out_resolution, bool *out_dirty);

argus_vector_2u_t argus_window_peek_resolution(argus_window_const_t self);

void argus_window_set_windowed_resolution(argus_window_t self, uint32_t width, uint32_t height);

void argus_window_is_vsync_enabled(argus_window_t self, bool *out_enabled, bool *out_dirty);

void argus_window_set_vsync_enabled(argus_window_t self, bool enabled);

void argus_window_set_windowed_position(argus_window_t self, int32_t x, int32_t y);

argus_display_const_t argus_window_get_display_affinity(argus_window_const_t self);

void argus_window_set_display_affinity(argus_window_t self, argus_display_const_t display);

argus_display_mode_t argus_window_get_display_mode(argus_window_const_t self);

void argus_window_set_display_mode(argus_window_t self, argus_display_mode_t mode);

bool argus_window_is_mouse_captured(argus_window_const_t self);

void argus_window_set_mouse_captured(argus_window_t self, bool captured);

bool argus_window_is_mouse_visible(argus_window_const_t self);

void argus_window_set_mouse_visible(argus_window_t self, bool visible);

bool argus_window_is_mouse_raw_input(argus_window_const_t self);

void argus_window_set_mouse_raw_input(argus_window_t self, bool raw_input);

argus_vector_2f_t argus_window_get_content_scale(argus_window_const_t self);

void argus_window_set_close_callback(argus_window_t self, argus_window_callback_t callback);

void argus_window_commit(argus_window_t self);

void argus_window_request_close(argus_window_t self);

#ifdef __cplusplus
}
#endif

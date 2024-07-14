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

#include "argus/lowlevel/cabi/math.h"

#include "argus/wm/display.hpp"
#include "argus/wm/window.hpp"
#include "argus/wm/cabi/display.h"
#include "argus/wm/cabi/window.h"
#include "internal/wm/display.hpp"

using argus::Window;

static Window &_as_ref(argus_window_t ptr) {
    return *reinterpret_cast<Window *>(ptr);
}

static const Window &_as_ref(argus_window_const_t ptr) {
    return *reinterpret_cast<const Window *>(ptr);
}

#ifdef __cplusplus
extern "C" {
#endif

void argus_set_window_creation_flags(WindowCreateFlags flags) {
    argus::set_window_creation_flags(argus::WindowCreationFlags(flags));
}

argus_window_t argus_get_window(const char *id) {
    return argus::get_window(id);
}

void *argus_get_window_handle(argus_window_const_t window) {
    return argus::get_window_handle(*reinterpret_cast<const Window *>(window));
}

argus_window_t argus_get_window_from_handle(const void *handle) {
    return argus::get_window_from_handle(handle);
}

void argus_window_set_canvas_ctor_and_dtor(argus_canvas_ctor_t ctor, argus_canvas_dtor_t dtor) {
    Window::set_canvas_ctor_and_dtor(
            [ctor](Window &window) -> argus::Canvas & { return *reinterpret_cast<argus::Canvas *>(ctor(&window)); },
            [dtor](argus::Canvas &canvas) { dtor(&canvas); }
    );
}

argus_window_t argus_window_create(const char *id, argus_window_t parent) {
    return &Window::create(id, reinterpret_cast<Window *>(parent));
}

const char *argus_window_get_id(argus_window_const_t self) {
    return _as_ref(self).get_id().c_str();
}

argus_canvas_t argus_window_get_canvas(argus_window_const_t self) {
    return &_as_ref(self).get_canvas();
}

bool argus_window_is_created(argus_window_const_t self) {
    return _as_ref(self).is_created();
}

bool argus_window_is_ready(argus_window_const_t self) {
    return _as_ref(self).is_ready();
}

bool argus_window_is_closed(argus_window_const_t self) {
    return _as_ref(self).is_closed();
}

argus_window_t argus_window_create_child_window(argus_window_t self, const char *id) {
    return &_as_ref(self).create_child_window(id);
}

void argus_window_remove_child(argus_window_t self, argus_window_const_t child) {
    _as_ref(self).remove_child(_as_ref(child));
}

void argus_window_update(argus_window_t self, uint64_t delta_us) {
    _as_ref(self).update(std::chrono::microseconds(delta_us));
}

void argus_window_set_title(argus_window_t self, const char *title) {
    _as_ref(self).set_title(title);
}

bool argus_window_is_fullscreen(argus_window_const_t self) {
    return _as_ref(self).is_fullscreen();
}

void argus_window_set_fullscreen(argus_window_t self, bool fullscreen) {
    _as_ref(self).set_fullscreen(fullscreen);
}

void argus_window_get_resolution(argus_window_t self, argus_vector_2u_t *out_resolution, bool *out_dirty) {
    auto res = _as_ref(self).get_resolution();
    *out_resolution = as_c_vec(res.value);
    *out_dirty = res.dirty;
}

argus_vector_2u_t argus_window_peek_resolution(argus_window_const_t self) {
    auto res = _as_ref(self).peek_resolution();
    return argus::as_c_vec(res);
}

void argus_window_set_windowed_resolution(argus_window_t self, uint32_t width, uint32_t height) {
    _as_ref(self).set_windowed_resolution(width, height);
}

void argus_window_is_vsync_enabled(argus_window_t self, bool *out_enabled, bool *out_dirty) {
    auto res = _as_ref(self).is_vsync_enabled();
    *out_enabled = res.value;
    *out_dirty = res.dirty;
}

void argus_window_set_vsync_enabled(argus_window_t self, bool enabled) {
    _as_ref(self).set_vsync_enabled(enabled);
}

void argus_window_set_windowed_position(argus_window_t self, int32_t x, int32_t y) {
    _as_ref(self).set_windowed_position(x, y);
}

argus_display_const_t argus_window_get_display_affinity(argus_window_const_t self) {
    return reinterpret_cast<argus_display_const_t>(&_as_ref(self).get_display_affinity());
}

void argus_window_set_display_affinity(argus_window_t self, argus_display_const_t display) {
    _as_ref(self).set_display_affinity(*reinterpret_cast<const argus::Display *>(display));
}

argus_display_mode_t argus_window_get_display_mode(argus_window_const_t self) {
    return argus::as_c_display_mode(_as_ref(self).get_display_mode());
}

void argus_window_set_display_mode(argus_window_t self, argus_display_mode_t mode) {
    _as_ref(self).set_display_mode(argus::as_cpp_display_mode(mode));
}

bool argus_window_is_mouse_captured(argus_window_const_t self) {
    return _as_ref(self).is_mouse_captured();
}

void argus_window_set_mouse_captured(argus_window_t self, bool captured) {
    _as_ref(self).set_mouse_captured(captured);
}

bool argus_window_is_mouse_visible(argus_window_const_t self) {
    return _as_ref(self).is_mouse_visible();
}

void argus_window_set_mouse_visible(argus_window_t self, bool visible) {
    _as_ref(self).set_mouse_visible(visible);
}

bool argus_window_is_mouse_raw_input(argus_window_const_t self) {
    return _as_ref(self).is_mouse_raw_input();
}

void argus_window_set_mouse_raw_input(argus_window_t self, bool raw_input) {
    _as_ref(self).set_mouse_raw_input(raw_input);
}

argus_vector_2f_t argus_window_get_content_scale(argus_window_const_t self) {
    auto scale = _as_ref(self).get_content_scale();
    return argus::as_c_vec(scale);
}

void argus_window_set_close_callback(argus_window_t self, argus_window_callback_t callback) {
    _as_ref(self).set_close_callback([callback](Window &window) { callback(&window); });
}

void argus_window_commit(argus_window_t self) {
    _as_ref(self).commit();
}

void argus_window_request_close(argus_window_t self) {
    _as_ref(self).request_close();
}

#ifdef __cplusplus
}
#endif

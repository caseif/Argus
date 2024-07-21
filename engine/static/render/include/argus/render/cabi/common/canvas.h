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

#include "argus/render/cabi/2d/attached_viewport_2d.h"
#include "argus/render/cabi/common/attached_viewport.h"
#include "argus/render/cabi/common/viewport.h"

#include "argus/wm/cabi/window.h"

#include <stdint.h>

typedef void *argus_canvas_t;
typedef const void *argus_canvas_const_t;

argus_window_t argus_canvas_get_window(argus_canvas_const_t canvas);

size_t argus_canvas_get_viewports_2d_count(argus_canvas_const_t canvas);

void argus_canvas_get_viewports_2d(argus_canvas_const_t canvas, argus_attached_viewport_2d_t *dest, size_t count);

argus_attached_viewport_t argus_canvas_find_viewport(argus_canvas_const_t canvas, const char *id);

argus_attached_viewport_2d_t argus_canvas_attach_viewport_2d(argus_canvas_t canvas, const char *id,
        ArgusViewport viewport, argus_camera_2d_t camera, uint32_t z_index);

argus_attached_viewport_2d_t argus_canvas_attach_default_viewport_2d(argus_canvas_t canvas, const char *id,
        argus_camera_2d_t camera, uint32_t z_index);

void argus_canvas_detach_viewport_2d(argus_canvas_t canvas, const char *id);

#ifdef __cplusplus
}
#endif

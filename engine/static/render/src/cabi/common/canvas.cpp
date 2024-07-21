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

#include "argus/render/cabi/common/canvas.h"

#include "argus/render/common/canvas.hpp"

#include "argus/core/engine.hpp"

#include "argus/lowlevel/debug.hpp"

static inline argus::Canvas &_as_ref(argus_canvas_t canvas) {
    return *reinterpret_cast<argus::Canvas *>(canvas);
}

static inline const argus::Canvas &_as_ref(argus_canvas_const_t canvas) {
    return *reinterpret_cast<const argus::Canvas *>(canvas);
}

extern "C" {

argus_window_t argus_canvas_get_window(argus_canvas_const_t canvas) {
    return &_as_ref(canvas).get_window();
}

size_t argus_canvas_get_viewports_2d_count(argus_canvas_const_t canvas) {
    return _as_ref(canvas).get_viewports_2d().size();
}

void argus_canvas_get_viewports_2d(argus_canvas_const_t canvas, argus_attached_viewport_2d_t *dest,
        size_t count) {
    auto viewports = _as_ref(canvas).get_viewports_2d();
    affirm_precond(count == viewports.size(), "Call to argus_canvas_get_viewports_2d with wrong count parameter");

    for (size_t i = 0; i < count; i++) {
        dest[i] = &viewports.at(i);
    }
}

argus_attached_viewport_2d_t argus_canvas_find_viewport(argus_canvas_const_t canvas, const char *id) {
    auto res = _as_ref(canvas).find_viewport(id);
    if (res.has_value()) {
        return &res.value().get();
    } else {
        return nullptr;
    }
}

argus_attached_viewport_2d_t argus_canvas_attach_viewport_2d(argus_canvas_t canvas, const char *id,
        ArgusViewport viewport, argus_camera_2d_t camera, uint32_t z_index) {
    return &_as_ref(canvas).attach_viewport_2d(id, *reinterpret_cast<argus::Viewport *>(&viewport),
            *reinterpret_cast<argus::Camera2D *>(camera), z_index);
}

argus_attached_viewport_2d_t argus_canvas_attach_default_viewport_2d(argus_canvas_t canvas, const char *id,
        argus_camera_2d_t camera, uint32_t z_index) {
    return &_as_ref(canvas).attach_default_viewport_2d(id,
            *reinterpret_cast<argus::Camera2D *>(camera), z_index);
}

void argus_canvas_detach_viewport_2d(argus_canvas_t canvas, const char *id) {
    _as_ref(canvas).detach_viewport_2d(id);
}

}

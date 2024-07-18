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

#include "argus/render/cabi/2d/camera_2d.h"
#include "internal/render/cabi/common/transform.hpp"

#include "argus/render/2d/camera_2d.hpp"

static inline argus::Camera2D &_as_ref(argus_camera_2d_t camera) {
    return *reinterpret_cast<argus::Camera2D *>(camera);
}

static inline const argus::Camera2D &_as_ref(argus_camera_2d_const_t camera) {
    return *reinterpret_cast<const argus::Camera2D *>(camera);
}

extern "C" {

const char *argus_camera_2d_get_id(argus_camera_2d_const_t camera) {
    return _as_ref(camera).get_id().c_str();
}

argus_scene_2d_t argus_camera_2d_get_scene(argus_camera_2d_const_t camera) {
    return &_as_ref(camera).get_scene();
}

ArgusTransform2d argus_camera_2d_peek_transform(argus_camera_2d_const_t camera) {
    return wrap_transform_2d(_as_ref(camera).peek_transform());
}

ArgusTransform2d argus_camera_2d_get_transform(argus_camera_2d_t camera, bool *out_dirty) {
    auto res = _as_ref(camera).get_transform();
    *out_dirty = res.dirty;
    return wrap_transform_2d(res.value);
}

void argus_camera_2d_set_transform(argus_camera_2d_t camera, ArgusTransform2d transform) {
    _as_ref(camera).set_transform(unwrap_transform_2d(transform));
}

}

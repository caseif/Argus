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

#include "argus/render/cabi/2d/render_group_2d.h"
#include "argus/render/cabi/2d/render_object_2d.h"
#include "internal/render/cabi/common/transform.hpp"


#include "argus/lowlevel/cabi/handle.h"
#include "argus/lowlevel/cabi/math/vector.h"
#include "internal/lowlevel/cabi/handle.hpp"

#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include "argus/lowlevel/math/vector.hpp"

static inline argus::RenderObject2D &_as_ref(argus_render_object_2d_t obj) {
    return *reinterpret_cast<argus::RenderObject2D *>(obj);
}

static inline const argus::RenderObject2D &_as_ref(argus_render_object_2d_const_t obj) {
    return *reinterpret_cast<const argus::RenderObject2D *>(obj);
}

extern "C" {

ArgusHandle argus_render_object_2d_get_handle(argus_render_object_2d_const_t obj) {
    return wrap_handle(_as_ref(obj).get_handle());
}

argus_scene_2d_const_t argus_render_object_2d_get_scene(argus_render_object_2d_const_t obj) {
    return &_as_ref(obj).get_scene();
}

argus_render_group_2d_const_t argus_render_object_2d_get_parent(argus_render_object_2d_const_t obj) {
    return &_as_ref(obj).get_parent();
}

const char *argus_render_object_2d_get_material(argus_render_object_2d_const_t obj) {
    return _as_ref(obj).get_material().c_str();
}

size_t argus_render_object_2d_get_primitives_count(argus_render_object_2d_const_t obj) {
    return _as_ref(obj).get_primitives().size();
}

argus_vector_2f_t argus_render_object_2d_get_anchor_point(argus_render_object_2d_const_t obj) {
    return *reinterpret_cast<const argus_vector_2f_t *>(&_as_ref(obj).get_anchor_point());
}

argus_vector_2f_t argus_render_object_2d_get_atlas_stride(argus_render_object_2d_const_t obj) {
    return *reinterpret_cast<const argus_vector_2f_t *>(&_as_ref(obj).get_atlas_stride());
}

uint32_t argus_render_object_2d_get_z_index(argus_render_object_2d_const_t obj) {
    return _as_ref(obj).get_z_index();
}

float argus_render_object_2d_get_light_opacity(argus_render_object_2d_const_t obj) {
    return _as_ref(obj).get_light_opacity();
}

void argus_render_object_2d_set_light_opacity(argus_render_object_2d_t obj, float opacity) {
    _as_ref(obj).set_light_opacity(opacity);
}

argus_vector_2u_t argus_render_object_2d_get_active_frame(argus_render_object_2d_t obj, bool *out_dirty) {
    auto res = _as_ref(obj).get_active_frame();
    *out_dirty = res.dirty;
    return *reinterpret_cast<argus_vector_2u_t *>(&res.value);
}

void argus_render_object_2d_set_active_frame(argus_render_object_2d_t obj, argus_vector_2u_t frame) {
    _as_ref(obj).set_active_frame(*reinterpret_cast<argus::Vector2u *>(&frame));
}

ArgusTransform2d argus_render_object_2d_peek_transform(argus_render_object_2d_const_t obj) {
    return wrap_transform_2d(_as_ref(obj).peek_transform());
}

ArgusTransform2d argus_render_object_2d_get_transform(argus_render_object_2d_t obj) {
    return wrap_transform_2d(_as_ref(obj).get_transform());
}

void argus_render_object_2d_set_transform(argus_render_object_2d_t obj, ArgusTransform2d transform) {
    _as_ref(obj).set_transform(unwrap_transform_2d(transform));
}

argus_render_object_2d_t argus_render_object_2d_copy(argus_render_object_2d_const_t obj,
        argus_render_group_2d_t parent) {
    return &_as_ref(obj).copy(*reinterpret_cast<argus::RenderGroup2D *>(parent));
}

}

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
#include "argus/render/cabi/2d/render_prim_2d.h"
#include "argus/render/cabi/common/transform.h"
#include "internal/render/cabi/common/transform.hpp"

#include "argus/lowlevel/cabi/handle.h"
#include "argus/lowlevel/cabi/math/vector.h"
#include "internal/lowlevel/cabi/handle.hpp"

#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include <algorithm>
#include <vector>

#include <cstddef>
#include <cstdint>

static inline argus::RenderGroup2D &_as_ref(argus_render_group_2d_t group) {
    return *reinterpret_cast<argus::RenderGroup2D *>(group);
}

static inline const argus::RenderGroup2D &_as_ref(argus_render_group_2d_const_t group) {
    return *reinterpret_cast<const argus::RenderGroup2D *>(group);
}

extern "C" {

ArgusHandle argus_render_group_2d_get_handle(argus_render_group_2d_const_t group) {
    return wrap_handle(_as_ref(group).get_handle());
}

argus_scene_2d_t argus_render_group_2d_get_scene(argus_render_group_2d_const_t group) {
    return &_as_ref(group).get_scene();
}

argus_render_group_2d_t get_parent(argus_render_group_2d_const_t group) {
    auto res = _as_ref(group).get_parent();
    if (res.has_value()) {
        return &res.value().get();
    } else {
        return nullptr;
    }
}

ArgusHandle argus_render_group_2d_add_group(argus_render_group_2d_t group, ArgusTransform2d transform) {
    return wrap_handle(_as_ref(group).add_group(unwrap_transform_2d(transform)));
}

ArgusHandle argus_render_group_2d_add_object(argus_render_group_2d_t group, const char *material,
        const RenderPrimitive2d *primitives, size_t primitives_count, argus_vector_2f_t anchor_point,
        argus_vector_2f_t atlas_stride, uint32_t z_index, float light_opacity, ArgusTransform2d transform) {
    std::vector<argus::RenderPrim2D> unwrapped_prims;
    std::transform(primitives, primitives + primitives_count, unwrapped_prims.end(), [](const auto &prim) {
        return argus::RenderPrim2D(std::vector<argus::Vertex2D>(
                reinterpret_cast<argus::Vertex2D *>(prim.vertices),
                reinterpret_cast<argus::Vertex2D *>(prim.vertices) + prim.vertex_count));
    });

    return wrap_handle(_as_ref(group).add_object(material, unwrapped_prims,
            *reinterpret_cast<argus::Vector2f *>(&anchor_point),
            *reinterpret_cast<argus::Vector2f *>(&atlas_stride),
            z_index, light_opacity, unwrap_transform_2d(transform)));
}

void argus_render_group_2d_remove_group(argus_render_group_2d_t group, ArgusHandle handle) {
    _as_ref(group).remove_group(unwrap_handle(handle));
}

void argus_render_group_2d_remove_object(argus_render_group_2d_t group, ArgusHandle handle) {
    _as_ref(group).remove_group(unwrap_handle(handle));
}

ArgusTransform2d argus_render_group_2d_peek_transform(argus_render_group_2d_const_t group) {
    return wrap_transform_2d(_as_ref(group).peek_transform());
}

ArgusTransform2d argus_render_group_2d_get_transform(argus_render_group_2d_t group) {
    return wrap_transform_2d(_as_ref(group).get_transform());
}

void argus_render_group_2d_set_transform(argus_render_group_2d_t group, ArgusTransform2d transform) {
    _as_ref(group).set_transform(unwrap_transform_2d(transform));
}

argus_render_group_2d_t argus_render_group_2d_copy(argus_render_group_2d_t group) {
    return &_as_ref(group).copy();
}

}

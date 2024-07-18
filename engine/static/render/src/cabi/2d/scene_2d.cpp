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

#include "argus/render/cabi/2d/scene_2d.h"
#include "argus/render/cabi/common/transform.h"
#include "internal/render/cabi/common/transform.hpp"

#include "argus/lowlevel/cabi/handle.h"
#include "argus/lowlevel/cabi/math/vector.h"
#include "internal/lowlevel/cabi/handle.hpp"

#include "argus/render/2d/light_2d.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/render_prim_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "argus/render/common/transform.hpp"

#include "argus/core/engine.hpp"

#include "argus/lowlevel/debug.hpp"

#include <algorithm>
#include <vector>

#include <cstddef>
#include <cstdint>

static inline argus::Scene2D &_as_ref(argus_scene_2d_t scene) {
    return *reinterpret_cast<argus::Scene2D *>(scene);
}

static inline const argus::Scene2D &_as_ref(argus_scene_2d_const_t scene) {
    return *reinterpret_cast<const argus::Scene2D *>(scene);
}

extern "C" {

argus_scene_2d_t argus_scene_2d_create(const char *id) {
    return &argus::Scene2D::create(id);
}

bool argus_scene_2d_is_lighting_enabled(argus_scene_2d_const_t scene) {
    return _as_ref(scene).is_lighting_enabled();
}

void argus_scene_2d_set_lighting_enabled(argus_scene_2d_t scene, bool enabled) {
    _as_ref(scene).set_lighting_enabled(enabled);
}

float argus_scene_2d_peek_ambient_light_level(argus_scene_2d_const_t scene) {
    return _as_ref(scene).peek_ambient_light_level();
}

float argus_scene_2d_get_ambient_light_level(argus_scene_2d_t scene, bool *out_dirty) {
    auto res = _as_ref(scene).get_ambient_light_level();
    *out_dirty = res.dirty;
    return res.value;
}

void argus_scene_2d_set_ambient_light_level(argus_scene_2d_t scene, float level) {
    _as_ref(scene).set_ambient_light_level(level);
}

argus_vector_3f_t argus_scene_2d_peek_ambient_light_color(argus_scene_2d_const_t scene) {
    return *reinterpret_cast<const argus_vector_3f_t *>(&_as_ref(scene).peek_ambient_light_color());
}

argus_vector_3f_t argus_scene_2d_get_ambient_light_color(argus_scene_2d_t scene, bool *out_dirty) {
    auto res = _as_ref(scene).get_ambient_light_color();
    *out_dirty = res.dirty;
    return *reinterpret_cast<const argus_vector_3f_t *>(&res.value);
}

void argus_scene_2d_set_ambient_light_color(argus_scene_2d_t scene, argus_vector_3f_t color) {
    _as_ref(scene).set_ambient_light_color(*reinterpret_cast<argus::Vector3f *>(&color));
}

size_t get_lights_count(argus_scene_2d_t scene) {
    return _as_ref(scene).get_lights().size();
}

void get_lights(argus_scene_2d_t scene, argus_light_2d_t *dest, size_t count) {
    const auto lights = _as_ref(scene).get_lights();
    affirm_precond(count == lights.size(), "argus_scene_2d_get_lights called with wrong count parameter");
    std::transform(lights.begin(), lights.end(), dest, [](auto light_ref) { return &light_ref.get(); });
}

size_t get_lights_count_for_render(argus_scene_2d_t scene) {
    return _as_ref(scene).get_lights_for_render().size();
}

void get_lights_for_render(argus_scene_2d_t scene, argus_light_2d_t *dest, size_t count) {
    const auto lights = _as_ref(scene).get_lights();
    affirm_precond(count == lights.size(), "argus_scene_2d_get_lights_for_render called with wrong count parameter");
    std::transform(lights.begin(), lights.end(), dest, [](auto light_ref) { return &light_ref.get(); });
}

ArgusHandle argus_scene_2d_add_light(argus_scene_2d_t scene, Light2dType type, bool is_occludable,
        argus_vector_3f_t color, LightParameters params, ArgusTransform2d initial_transform) {
    return wrap_handle(_as_ref(scene).add_light(
            argus::Light2DType(type),
            is_occludable,
            *reinterpret_cast<argus::Vector3f *>(&color),
            *reinterpret_cast<argus::LightParameters *>(&params),
            unwrap_transform_2d(initial_transform)
    ));
}

argus_light_2d_t argus_scene_2d_get_light(argus_scene_2d_t scene, ArgusHandle handle) {
    auto res = _as_ref(scene).get_light(unwrap_handle(handle));
    if (res.has_value()) {
        return &res.value().get();
    } else {
        return nullptr;
    }
}

void argus_scene_2d_remove_light(argus_scene_2d_t scene, ArgusHandle handle) {
    _as_ref(scene).remove_light(unwrap_handle(handle));
}

argus_render_group_2d_t argus_scene_2d_get_group(argus_scene_2d_t scene, ArgusHandle handle) {
    auto res = _as_ref(scene).get_group(unwrap_handle(handle));
    if (res.has_value()) {
        return &res.value().get();
    } else {
        return nullptr;
    }
}

argus_render_object_2d_t argus_scene_2d_get_object(argus_scene_2d_t scene, ArgusHandle handle) {
    auto res = _as_ref(scene).get_object(unwrap_handle(handle));
    if (res.has_value()) {
        return &res.value().get();
    } else {
        return nullptr;
    }
}

ArgusHandle argus_scene_2d_add_group(argus_scene_2d_t scene, ArgusTransform2d transform) {
    return wrap_handle(_as_ref(scene).add_group(unwrap_transform_2d(transform)));
}

ArgusHandle argus_scene_2d_add_object(argus_scene_2d_t scene, const char *material, RenderPrimitive2d *primitives,
        size_t primitives_count, argus_vector_2f_t anchor_point, argus_vector_2f_t atlas_stride, uint32_t z_index,
        float light_opacity, ArgusTransform2d transform) {
    std::vector<argus::RenderPrim2D> unwrapped_prims;
    std::transform(primitives, primitives + primitives_count, unwrapped_prims.end(), [](const auto &prim) {
        return argus::RenderPrim2D(std::vector<argus::Vertex2D>(
                reinterpret_cast<argus::Vertex2D *>(prim.vertices),
                reinterpret_cast<argus::Vertex2D *>(prim.vertices) + prim.vertex_count));
    });
    return wrap_handle(_as_ref(scene).add_object(
            material,
            unwrapped_prims,
            *reinterpret_cast<argus::Vector2f *>(&anchor_point),
            *reinterpret_cast<argus::Vector2f *>(&atlas_stride),
            z_index,
            light_opacity,
            unwrap_transform_2d(transform)
    ));
}

void argus_scene_2d_remove_group(argus_scene_2d_t scene, ArgusHandle handle) {
    _as_ref(scene).remove_group(unwrap_handle(handle));
}

void argus_scene_2d_remove_object(argus_scene_2d_t scene, ArgusHandle handle) {
    _as_ref(scene).remove_object(unwrap_handle(handle));
}

argus_camera_2d_t find_camera(argus_scene_2d_const_t scene, const char *id) {
    auto res = _as_ref(scene).find_camera(id);
    if (res.has_value()) {
        return &res.value().get();
    } else {
        return nullptr;
    }
}

argus_camera_2d_t argus_scene_2d_create_camera(argus_scene_2d_t scene, const char *id) {
    return &_as_ref(scene).create_camera(id);
}

void argus_scene_2d_destroy_camera(argus_scene_2d_t scene, const char *id) {
    _as_ref(scene).destroy_camera(id);
}

void argus_scene_2d_lock_render_state(argus_scene_2d_t scene) {
    _as_ref(scene).lock_render_state();
}

void argus_scene_2d_unlock_render_state(argus_scene_2d_t scene) {
    _as_ref(scene).unlock_render_state();
}

}

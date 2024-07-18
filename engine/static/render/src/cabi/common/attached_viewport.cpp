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

#include "argus/render/cabi/common/attached_viewport.h"

#include "argus/render/common/attached_viewport.hpp"

#include "argus/core/engine.hpp"

#include "argus/lowlevel/debug.hpp"

#include <algorithm>

#include <cstddef>
#include <cstdint>

using argus::AttachedViewport;

static inline AttachedViewport &_as_ref(argus_attached_viewport_t viewport) {
    return *reinterpret_cast<AttachedViewport *>(viewport);
}

static inline const AttachedViewport &_as_ref(argus_attached_viewport_const_t viewport) {
    return *reinterpret_cast<const AttachedViewport *>(viewport);
}

extern "C" {

SceneType argus_attached_viewport_get_type(argus_attached_viewport_const_t viewport) {
    return SceneType(_as_ref(viewport).m_type);
}

Viewport argus_attached_viewport_get_viewport(argus_attached_viewport_const_t viewport) {
    const auto real_viewport = _as_ref(viewport).get_viewport();
    return *reinterpret_cast<const Viewport *>(&real_viewport);
}

uint32_t argus_attached_viewport_get_z_index(argus_attached_viewport_const_t viewport) {
    return _as_ref(viewport).get_z_index();
}

size_t argus_attached_viewport_get_postprocessing_shaders_count(argus_attached_viewport_const_t viewport) {
    return _as_ref(viewport).get_postprocessing_shaders().size();
}

void argus_attached_viewport_get_postprocessing_shaders(argus_attached_viewport_const_t viewport,
        const char **dest, size_t count) {
    const auto shaders = _as_ref(viewport).get_postprocessing_shaders();
    affirm_precond(count == shaders.size(),
            "Call to argus_attached_viewport_get_postprocessing_shaders with wrong count parameter");

    std::transform(shaders.begin(), shaders.end(), dest, [](const auto &uid) { return uid.c_str(); });
}

void argus_attached_viewport_add_postprocessing_shader(argus_attached_viewport_t viewport, const char *shader_uid) {
    _as_ref(viewport).add_postprocessing_shader(shader_uid);
}

void argus_attached_viewport_remove_postprocessing_shader(argus_attached_viewport_t viewport, const char *shader_uid) {
    _as_ref(viewport).remove_postprocessing_shader(shader_uid);
}

}

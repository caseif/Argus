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

#include "argus/render/cabi/common/material.h"

#include "argus/render/common/material.hpp"

#include "argus/core/engine.hpp"

#include "argus/lowlevel/debug.hpp"

#include <algorithm>

#include <cstddef>

static inline const argus::Material &_as_ref(argus_material_t material) {
    return *reinterpret_cast<const argus::Material *>(material);
}

extern "C" {

const char *argus_material_get_texture_uid(argus_material_t material) {
    return _as_ref(material).get_texture_uid().c_str();
}

size_t argus_material_get_shader_uids_count(argus_material_t material) {
    return _as_ref(material).get_shader_uids().size();
}

void argus_material_get_shader_uids(argus_material_t material, const char **out_uids, size_t count) {
    const auto &shaders = _as_ref(material).get_shader_uids();
    affirm_precond(count == shaders.size(),
            "Call to argus_material_get_shader_uids with wrong count parameter");
    std::transform(shaders.begin(), shaders.end(), out_uids,
            [](const auto &uid) { return uid.c_str(); });
}

}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/memory.hpp"

#include "argus/render/common/material.hpp"
#include "internal/render/pimpl/common/material.hpp"

#include <string>
#include <vector>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_Material));

    Material::Material(const std::string &texture, const std::vector<std::string> &shaders):
        pimpl(&g_pimpl_pool.construct<pimpl_Material>(texture, shaders)) {
    }

    Material::Material(const Material &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_Material>(*rhs.pimpl)) {
    }

    Material::Material(Material &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Material::~Material(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    const std::string &Material::get_texture_uid(void) const {
        return pimpl->texture;
    }

    const std::vector<std::string> &Material::get_shader_uids(void) const {
        return pimpl->shaders;
    }
}

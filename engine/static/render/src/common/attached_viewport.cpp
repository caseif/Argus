/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "argus/render/common/attached_viewport.hpp"
#include "internal/render/pimpl/common/attached_viewport.hpp"

#include <string>
#include <vector>

namespace argus {
    AttachedViewport::AttachedViewport(SceneType type):
            type(type) {
    }

    AttachedViewport::~AttachedViewport(void) {
    }


    Viewport AttachedViewport::get_viewport(void) const {
        return get_pimpl()->viewport;
    }

    uint32_t AttachedViewport::get_z_index(void) const {
        return get_pimpl()->z_index;
    }

    std::vector<std::string> AttachedViewport::get_postprocessing_shaders(void) const {
        return get_pimpl()->postfx_shader_uids;
    }

    void AttachedViewport::add_postprocessing_shader(const std::string &shader_uid) {
        get_pimpl()->postfx_shader_uids.push_back(shader_uid);
    }

    void AttachedViewport::remove_postprocessing_shader(const std::string &shader_uid) {
        auto uids = get_pimpl()->postfx_shader_uids;
        auto it = std::find(uids.crbegin(), uids.crend(), shader_uid);
        if (it != uids.crend()) {
            // some voodoo because it's a reverse iterator
            std::advance(it, 1);
            uids.erase(it.base());
        }
    }
}
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

#pragma once

#include "argus/render/common/viewport.hpp"

#include <string>
#include <vector>

#include <cstdint>

namespace argus {
    struct pimpl_AttachedViewport {
        protected:
        pimpl_AttachedViewport(const Viewport &viewport, uint32_t z_index):
            viewport(viewport),
            z_index(z_index) {
        }

        public:
        const Viewport viewport;
        const uint32_t z_index;

        std::vector<std::string> postfx_shader_uids;
    };
}

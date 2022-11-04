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

#pragma once

#include "argus/lowlevel/math.hpp"

#include "argus/render/common/attached_viewport.hpp"
#include "argus/render/common/viewport.hpp"

#include <string>
#include <vector>

#include <cstdint>

namespace argus {
    // forward declarations
    class Camera2D;
    struct pimpl_AttachedViewport2D;

    struct AttachedViewport2D : AttachedViewport {
        pimpl_AttachedViewport2D *pimpl;

        AttachedViewport2D(const Viewport &viewport, Camera2D &camera, uint32_t z_index);

        AttachedViewport2D(const AttachedViewport2D&) = delete;

        AttachedViewport2D(AttachedViewport2D&&);

        ~AttachedViewport2D(void);

        pimpl_AttachedViewport *get_pimpl(void) const;

        Camera2D &get_camera(void) const;
    };
}

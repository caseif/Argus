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

#include "argus/ecs/entity.hpp"

#include <cstdint>

namespace argus {
    // forward declarations
    struct pimpl_World2D;

    class World2D {
        public:
            pimpl_World2D *pimpl;

            Entity &create_textured_background_layer(const std::string &id, uint8_t index,
                    const std::string &texture_uid, const Vector2f &base_offset, const Vector2f &parallax_coeff);
    };
}

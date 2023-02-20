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

#include "argus/lowlevel/math.hpp"

#include "argus/game2d/actor_2d.hpp"
#include "argus/game2d/sprite.hpp"
#include "argus/game2d/static_object_2d.hpp"
#include "argus/game2d/world2d.hpp"

#include <functional>
#include <optional>
#include <string>

#include <cstdint>

namespace argus {
    // forward declarations
    class World2D;

    struct pimpl_World2DLayer;

    class World2DLayer {
      public:
        pimpl_World2DLayer *pimpl;

        World2DLayer(World2D &world, const std::string &id, uint32_t z_index, float parallax_coeff,
                std::optional<Vector2f> repeat_interval);

        World2DLayer(const World2DLayer &) = delete;

        World2DLayer(World2DLayer &&) = delete;

        ~World2DLayer();

        World2D &get_world(void) const;

        StaticObject2D &get_static_object(Handle handle) const;

        Handle create_static_object(const std::string &sprite, const Vector2f &size, uint32_t z_index,
                const Transform2D &transform);

        void delete_static_object(Handle handle);

        Actor2D &get_actor(Handle handle) const;

        Handle create_actor(const std::string &sprite, const Vector2f &size, uint32_t z_index,
                const Transform2D &transform);

        void delete_actor(Handle handle);
    };
}

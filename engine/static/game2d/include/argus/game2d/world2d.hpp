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

#include "argus/render/2d/camera_2d.hpp"

#include "argus/game2d/actor_2d.hpp"
#include "argus/game2d/world2d_layer.hpp"

#include <optional>
#include <string>
#include <utility>

namespace argus {
    // forward declarations
    class Canvas;

    class Sprite;

    struct pimpl_World2D;

    class World2D : AutoCleanupable {
      public:
        static World2D &create(const std::string &id, Canvas &canvas, float scale_factor);

        static World2D &get(const std::string &id);

        pimpl_World2D *pimpl;

        World2D(const std::string &id, Canvas &canvas, float scale_factor);

        World2D(World2D &) = delete;

        World2D(World2D &&) = delete;

        ~World2D(void) override;

        const std::string &get_id(void) const;

        float get_scale_factor(void) const;

        const Transform2D &get_camera_transform(void) const;

        void set_camera_transform(const Transform2D &transform);

        World2DLayer &get_background_layer(uint32_t index) const;

        World2DLayer &add_background_layer(float parallax_coeff, std::optional<Vector2f> repeat_interval);

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

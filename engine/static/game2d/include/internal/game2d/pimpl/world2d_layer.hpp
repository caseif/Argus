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
#include "argus/render/2d/scene_2d.hpp"

#include "argus/game2d/actor_2d.hpp"
#include "argus/game2d/static_object_2d.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>

namespace argus {
    struct pimpl_World2DLayer {
        World2D &world;
        std::string id;

        uint32_t z_index;
        float parallax_coeff;
        std::optional<Vector2f> repeat_interval;

        Scene2D *scene;
        Camera2D *render_camera;

        std::set<Handle> static_objects;
        std::set<Handle> actors;

        pimpl_World2DLayer(World2D &world, const std::string &id, uint32_t z_index, float parallax_coeff,
                std::optional<Vector2f> repeat_interval) :
                world(world),
                id(id),
                z_index(z_index),
                parallax_coeff(parallax_coeff),
                repeat_interval(repeat_interval) {
        }
    };
}

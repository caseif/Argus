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

#include "argus/render/2d/scene_2d.hpp"
namespace argus {
    struct pimpl_Camera2D {
        std::string id;
        Scene2D &scene;
        AtomicDirtiable<Transform2D> transform;

        pimpl_Camera2D(const std::string &id, Scene2D &scene):
                id(id),
                scene(scene) {
        }
    };
}

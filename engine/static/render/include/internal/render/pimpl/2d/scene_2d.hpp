/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/pimpl/common/scene.hpp"

namespace argus {
    // forward declarations
    struct pimpl_Scene;

    struct pimpl_Scene2D : public pimpl_Scene {
        RenderGroup2D root_group;
        
        pimpl_Scene2D(const Canvas &canvas, Scene2D &scene, const Transform2D &transform, const int index):
                pimpl_Scene(canvas, transform, index),
                root_group(scene, nullptr) {
        }

        pimpl_Scene2D(const pimpl_Scene2D&) = default;

        pimpl_Scene2D(pimpl_Scene2D&&) = delete;
    };
}

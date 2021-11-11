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

// module render
#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"

#include <vector>

namespace argus {
    struct pimpl_Scene {
        const Canvas &canvas;
        Transform2D transform;
        const int index;
        
        pimpl_Scene(const Canvas &canvas, const Transform2D &transform, const int index):
                canvas(canvas),
                transform(transform),
                index(index) {
        }

        pimpl_Scene(const pimpl_Scene&) = default;

        pimpl_Scene(pimpl_Scene&&) = delete;
    };
}

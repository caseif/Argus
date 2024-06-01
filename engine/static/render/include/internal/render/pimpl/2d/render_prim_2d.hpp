/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/render/common/vertex.hpp"
#include "argus/render/2d/render_prim_2d.hpp"

#include <atomic>
#include <vector>

#include <cstddef>

namespace argus {
    struct pimpl_RenderPrim2D {
        /**
         * \brief The vertices comprising this RenderPrim.
         */
        const std::vector<Vertex2D> vertices;

        size_t frame_count;

        pimpl_RenderPrim2D(const std::vector<Vertex2D> &vertices) :
                vertices(vertices) {
        }

        pimpl_RenderPrim2D(const pimpl_RenderPrim2D &) = default;

        pimpl_RenderPrim2D(pimpl_RenderPrim2D &&) = delete;
    };
}

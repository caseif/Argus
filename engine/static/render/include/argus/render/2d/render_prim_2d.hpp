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

#include "argus/lowlevel/math.hpp"

#include "argus/render/common/transform.hpp"
#include "argus/render/common/vertex.hpp"

#include <atomic>
#include <iterator>
#include <string>
#include <vector>

#include <cstddef>
#include <cstdio>

namespace argus {
    // forward declarations

    struct pimpl_RenderPrim2D;

    /**
     * @brief Represents primitive 2D geometry to be rendered.
     */
    class RenderPrim2D {
      public:
        pimpl_RenderPrim2D *m_pimpl;

        /**
         * @brief Constructs a new RenderPrim2D object.
         *
         * @param vertices The vertices comprising the new primitive.
         */
        RenderPrim2D(const std::vector<Vertex2D> &vertices);

        /**
         * @brief Constructs a new RenderPrim2D object.
         *
         * @param vertices The vertices comprising the new primitive.
         */
        RenderPrim2D(std::initializer_list<Vertex2D> vertices);

        RenderPrim2D(void) = delete;

        RenderPrim2D(const RenderPrim2D &) noexcept;

        RenderPrim2D(RenderPrim2D &&) noexcept;

        RenderPrim2D &operator=(const RenderPrim2D &);

        RenderPrim2D &operator=(RenderPrim2D &&);

        ~RenderPrim2D(void);

        /**
         * @brief Gets the current vertex count of this RenderPrim2D.
         *
         * @return The current vertex count of this RenderPrim2D.
         */
        [[nodiscard]] size_t get_vertex_count(void) const;

        [[nodiscard]] const std::vector<Vertex2D> &get_vertices(void) const;

        /**
         * @brief Gets the number of unique animation frames of this
         *        RenderPrim2D.
         *
         * @return The number of unique animation frames.
         */
        [[nodiscard]] size_t get_frame_count(void) const;
    };
}

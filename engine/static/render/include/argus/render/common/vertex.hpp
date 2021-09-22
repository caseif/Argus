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

#include "argus/lowlevel/math.hpp"

namespace argus {
    /**
     * \brief Represents a vertex in 2D space containing a 2-dimensional spatial
     *        position and normal vector, an RGBA color value, and 2-dimensional
     *        texture UV coordinates.
     */
    struct Vertex2D {
        /**
         * \brief The position of this vertex in 2D space.
         */
        Vector2f position;
        /**
         * \brief The normal of this vertex in 2D space.
         */
        Vector2f normal;
        /**
         * \brief The RGBA color of this vertex in [0,1] space.
         */
        Vector4f color;
        /**
         * \brief The texture coordinates of this vertex in UV-space.
         */
        Vector2f tex_coord;
    };

    /**
     * \brief Represents a vertex in 3D space containing a 3-dimensional spatial
     *        position and normal vector, an RGBA color value, and 2-dimensional
     *        texture UV coordinates.
     */
    struct Vertex3D {
        /**
         * \brief The position of this vertex in 2D space.
         */
        Vector3f position;
        /**
         * \brief The normal of this vertex in 2D space.
         */
        Vector3f normal;
        /**
         * \brief The RGBA color of this vertex in [0,1] space.
         */
        Vector4f color;
        /**
         * \brief The texture coordinates of this vertex in UV-space.
         */
        Vector2f tex_coord;
    };
}

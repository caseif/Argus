/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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
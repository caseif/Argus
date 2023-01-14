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
namespace argus {
    /**
     * \brief Specifies the conversion from viewport coordinate space to
     *        pixel space for each axis with respect to surface aspect ratio.
     *
     * This can be conceptualized as defining where the values of 0 and 1 are
     * placed on the physical surface for each axis.
     */
    enum class ViewportCoordinateSpaceMode {
        /**
         * \brief Each axis will be scaled independently, with 0 and 1 being on
         *        opposite edges of the surface.
         */
        Individual,
        /**
         * \brief Both axes will be scaled relative to the smaller of the two
         *        axes.
         *
         * If the axes are not equal in length, the edges of the viewport on the
         * larger dimension will stop short of the edges of the surface.
         */
        MinAxis,
        /**
         * \brief Both axes will be scaled relative to the larger of the two
         *        axes.
         *
         * If the axes are not equal in length, the edges of the viewport on the
         * smaller dimension will fall outside of the bounds of the surface.
         */
        MaxAxis,
        /**
         * \brief Both axes will be scaled relative to the horizontal axis,
         *        regardless of which dimension is larger.
         */
        HorizontalAxis,
        /**
         * \brief Both axes will be scaled relative to the vertical axis,
         *        regardless of which dimension is larger.
         */
        VerticalAxis
    };

    struct Viewport {
        float top;
        float bottom;
        float left;
        float right;

        Vector2f scaling;
        ViewportCoordinateSpaceMode mode;
    };
}

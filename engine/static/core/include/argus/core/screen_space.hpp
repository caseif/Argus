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

namespace argus {
    struct ScreenSpace {
        float left;
        float right;
        float top;
        float bottom;
    };

    static_assert(std::is_pod_v<ScreenSpace>);

    /**
     * \brief Controls how the screen space is scaled with respect to window
     *        aspect ratio.
     *
     * When configured as any value other than `None`, either the horizontal or
     * vertical axis will be "normalized" while the other is scaled. The normal
     * axis will maintain the exact bounds requested by the provided screen
     * space configuration, while the other will have its bounds changed such
     * that the aspect ratio of the screen space matches that of the window.
     * If the window is resized, the screen space will be updated in tandem.
     */
    enum class ScreenSpaceScaleMode {
        /**
         * \brief Normalizes the screen space dimension with the minimum range.
         *
         * The bounds of the smaller window dimension will be exactly as
         * configured. Meanwhile, the bounds of the larger dimension will be
         * "extended" beyond what they would be in a square (1:1 aspect ratio)
         * window such that regions become visible which would otherwise not be
         * in the square window.
         *
         * For example, a typical computer monitor is wider than it is tall, so
         * given this mode, the bounds of the screen space of a fullscreen
         * window would be preserved in the vertical dimension, while the bounds
         * in the horizontal dimension would be larger than usual (+/- 1.778 on
         * a 16:9 monitor, exactly the ratio of the monitor dimensions).
         *
         * Meanwhile, a phone screen (held upright) is taller than it is wide,
         * so it would see the bounds of the screen space extended in the
         * vertical dimension instead.
         */
        NormalizeMinDimension,
        /**
         * \brief Normalizes the screen space dimension with the maximum range.
         *
         * This is effectively the inverse of `NormalizedMinDimension`. The
         * bounds of the screen space are preserved on the larger dimension, and
         * "shrunk" on the smaller one. This effectively hides regions that
         * would be visible in a square window.
         */
        NormalizeMaxDimension,
        /**
         * \brief Normalizes the vertical screen space dimension.
         *
         * This invariably normalizes the vertical dimension of the screen
         * space regardless of which dimension is larger. The horizontal
         * dimension is grown or shrunk depending on the aspect ratio of the
         * window.
         */
        NormalizeVertical,
        /**
         * \brief Normalizes the horizontal screen space dimension.
         *
         * This invariably normalizes the horizontal dimension of the screen
         * space regardless of which dimension is larger. The vertical
         * dimension is grown or shrunk depending on the aspect ratio of the
         * window.
         */
        NormalizeHorizontal,
        /**
         * \brief Does not normalize screen space with respect to aspect ratio.
         *
         * Given an aspect ratio other than 1:1, the contents of the window will
         * be stretched in one dimension or the other depending on which is
         * larger.
         */
        None
    };
}

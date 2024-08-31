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

#include "argus/wm/window.hpp"

#include "argus/lowlevel/math.hpp"

namespace argus::input {
    enum class MouseButton {
        Primary = 1,
        Secondary = 2,
        Middle = 3,
        Back = 4,
        Forward = 5,
    };

    enum class MouseAxis {
        Horizontal,
        Vertical,
    };

    /**
     * @brief Returns the change in position of the mouse from the previous
     *        frame.
     */
    argus::Vector2d mouse_delta(void);

    /**
     * @brief Gets the current position of the mouse within the currently
     *        focused window.
     */
    argus::Vector2d mouse_pos(void);

    /**
     * @brief Gets the current value of the provided mouse axis.
     *
     * @param axis The axis to check.
     *
     * @return The value of the mouse axis.
     */
    double get_mouse_axis(MouseAxis axis);

    /**
     * @brief Gets the last delta of the provided mouse axis.
     *
     * @param axis The axis to check.
     *
     * @return The delta of the mouse axis.
     */
    double get_mouse_axis_delta(MouseAxis axis);

    /**
     * @brief Gets whether a mouse button is currently pressed.
     *
     * @param button The mouse button to check.
     *
     * @return Whether the mouse button is currently pressed.
     */
    bool is_mouse_button_pressed(MouseButton button);
}

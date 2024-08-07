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

    /**
     * @brief Sets whether the mouse cursor should be hidden and locked to the
     *        boundaries of the given window while it is focused.
     *
     * If set to false, the mouse cursor will be made visible regardless of
     * whether it was hidden prior to being captured.
     *
     * @param window The window to configure the cursor mode for.
     * @param captured Whether the cursor should be captured.
     */
    void set_mouse_captured(const argus::Window &window, bool captured);

    /**
     * @brief Sets whether the mouse cursor should be visible while inside the
     *        boundaries of the given window.
     *
     * If the cursor is currently captured for the window, this function will
     * have no effect.
     *
     * @param window The window to configure the cursor mode for.
     * @param visible Whether the cursor should be visible.
     */
    void set_mouse_visible(const argus::Window &window, bool visible);

    /**
     * @brief Sets whether raw input should be used for the given window.
     *
     * If raw input is not supported on the system, this function will have no
     * effect.
     *
     * @param window The window to configure the mouse input mode for.
     * @param raw_input Whether raw mouse input should be used.
     */
    void set_mouse_raw_input(const argus::Window &window, bool raw_input);
}

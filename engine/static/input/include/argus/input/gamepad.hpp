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

#include <cstdint>

namespace argus::input {
    // forward declarations
    class Controller;

    typedef int GamepadId;

    enum class GamepadButton {
        Unknown,
        A,
        B,
        X,
        Y,
        DpadUp,
        DpadDown,
        DpadLeft,
        DpadRight,
        LBumper,
        RBumper,
        LTrigger,
        RTrigger,
        LStick,
        RStick,
        L4,
        R4,
        L5,
        R5,
        Start,
        Back,
        Guide,
        Misc1,
        MaxValue,
    };

    enum class GamepadAxis {
        Unknown,
        LeftX,
        LeftY,
        RightX,
        RightY,
        LTrigger,
        RTrigger,
        Max
    };

    uint8_t get_connected_gamepad_count(void);

    uint8_t get_unattached_gamepad_count(void);

    bool is_gamepad_button_pressed(GamepadId gamepad, GamepadButton button);
}

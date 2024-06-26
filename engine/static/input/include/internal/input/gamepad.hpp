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

#include "argus/lowlevel/result.hpp"

#include "argus/input/gamepad.hpp"

namespace argus::input {
    struct GamepadState {
        uint64_t button_state = 0;
        std::array<double, size_t(GamepadAxis::MaxValue)> axis_state = {};
        std::array<double, size_t(GamepadAxis::MaxValue)> axis_deltas = {};
    };

    void update_gamepads(void);

    void flush_gamepad_deltas(void);

    [[nodiscard]] Result<void, std::string> assoc_gamepad(HidDeviceId id, const std::string &controller_name);

    [[nodiscard]] Result<HidDeviceId, std::string> assoc_first_available_gamepad(const std::string &controller_name);

    void unassoc_gamepad(HidDeviceId id);

    void deinit_gamepads(void);
}

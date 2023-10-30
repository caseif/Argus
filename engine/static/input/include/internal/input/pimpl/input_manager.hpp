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

#include "argus/input/controller.hpp"
#include "argus/input/gamepad.hpp"
#include "internal/input/gamepad.hpp"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace argus::input {
    struct pimpl_InputManager {
        std::unordered_map<std::string, Controller *> controllers;

        std::vector<HidDeviceId> available_gamepads;
        std::unordered_map<HidDeviceId, std::string> mapped_gamepads;
        std::recursive_mutex gamepads_mutex;
        bool are_gamepads_initted = false;

        std::unordered_map<HidDeviceId, std::atomic_int64_t> gamepad_button_states;
        std::unordered_map<HidDeviceId, std::atomic<std::array<double, size_t(GamepadAxis::MaxValue)>>>
                gamepad_axis_states;

        pimpl_InputManager(void) = default;
    };
}

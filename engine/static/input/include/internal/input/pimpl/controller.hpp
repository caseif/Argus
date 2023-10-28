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
#include "argus/input/keyboard.hpp"

#include "SDL_gamecontroller.h"

#include <map>

namespace argus::input {
    struct pimpl_Controller {
        std::string name;
        std::optional<int> attached_gamepad;
        bool was_gamepad_disconnected = false;

        std::map<KeyboardScancode, std::vector<std::string>> key_to_action_bindings;
        std::map<std::string, std::vector<KeyboardScancode>> action_to_key_bindings;

        std::map<MouseButton, std::vector<std::string>> mouse_button_to_action_bindings;
        std::map<std::string, std::vector<MouseButton>> action_to_mouse_button_bindings;

        std::map<MouseAxis, std::vector<std::string>> mouse_axis_to_action_bindings;
        std::map<std::string, std::vector<MouseAxis>> action_to_mouse_axis_bindings;

        std::map<GamepadButton, std::vector<std::string>> gamepad_button_to_action_bindings;
        std::map<std::string, std::vector<GamepadButton>> action_to_gamepad_button_bindings;

        std::map<GamepadAxis, std::vector<std::string>> gamepad_axis_to_action_bindings;
        std::map<std::string, std::vector<GamepadAxis>> action_to_gamepad_axis_bindings;

        pimpl_Controller(std::string name) :
            name(std::move(name)) {
        }
    };
}

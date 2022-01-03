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

#include "argus/input/controller.hpp"
#include "argus/input/keyboard.hpp"

#include <map>

namespace argus { namespace input {
    struct pimpl_Controller {
        ControllerIndex index;
        std::map<KeyboardScancode, std::vector<std::string>> key_to_action_bindings;
        std::map<std::string, std::vector<KeyboardScancode>> action_to_key_bindings;

        pimpl_Controller(ControllerIndex index):
                index(index) {
        }
    };
}}

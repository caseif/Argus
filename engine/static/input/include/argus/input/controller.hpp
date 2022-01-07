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

#include "argus/input/keyboard.hpp"

#include <string>
#include <vector>

namespace argus { namespace input {
    // forward declarations
    class InputManager;

    struct pimpl_Controller;

    typedef uint16_t ControllerIndex;

    class Controller {
        friend class InputManager;

        private:
            pimpl_Controller *pimpl;

            Controller(ControllerIndex index);

            Controller(Controller&) = delete;

            Controller(Controller&&) = delete;

            ~Controller(void);

        public:
            ControllerIndex get_index(void);

            void unbind_action(const std::string &action);

            const std::vector<std::string> get_keyboard_key_bindings(KeyboardScancode key);

            const std::vector<KeyboardScancode> get_keyboard_action_bindings(const std::string &action);

            void bind_keyboard_key(KeyboardScancode key, const std::string &action);

            void unbind_keyboard_key(KeyboardScancode key);

            void unbind_keyboard_key(KeyboardScancode key, const std::string &action);
    };
}}

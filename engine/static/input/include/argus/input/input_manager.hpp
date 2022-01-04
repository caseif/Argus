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

namespace argus { namespace input {
    struct pimpl_InputManager;

    class InputManager {
        private:
            pimpl_InputManager *pimpl;

            InputManager(void);

            InputManager(InputManager&) = delete;
            
            InputManager(InputManager&&) = delete;

            ~InputManager(void);
        public:
            static InputManager &instance(void);

            Controller &get_controller(ControllerIndex controller_index);

            Controller &add_controller(void);

            void remove_controller(Controller &controller);

            void remove_controller(ControllerIndex controller_index);

            void handle_key_press(const Window &window, KeyboardScancode key, bool release) const;
    };
}}

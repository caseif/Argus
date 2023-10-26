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

#include <string>

namespace argus::input {
    struct pimpl_InputManager;

    class InputManager : AutoCleanupable {
      private:

        InputManager(void);

        ~InputManager(void) override;

      public:
        pimpl_InputManager *pimpl;

        InputManager(InputManager &) = delete;

        InputManager(InputManager &&) = delete;

        static InputManager &instance(void);

        Controller &get_controller(const std::string &name);

        Controller &add_controller(const std::string &name, bool assign_gamepad);

        void remove_controller(Controller &controller);

        void remove_controller(const std::string &name);

        void handle_key_press(const Window &window, KeyboardScancode key, bool release) const;

        void handle_mouse_button_press(const Window &window, MouseButton button, bool release) const;

        void handle_mouse_axis_change(const Window &window, MouseAxis axis, double value, double delta) const;
    };
}

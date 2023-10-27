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

#include "argus/input/gamepad.hpp"
#include "argus/input/keyboard.hpp"
#include "argus/input/mouse.hpp"

#include <string>
#include <vector>

namespace argus::input {
    // forward declarations
    class InputManager;

    struct pimpl_Controller;

    class Controller : AutoCleanupable {
        friend class InputManager;

      private:
        Controller(const std::string &name, bool assign_gamepad);

        ~Controller(void) override;

      public:
        pimpl_Controller *pimpl;

        Controller(const Controller &) = delete;

        Controller(Controller &&) = delete;

        [[nodiscard]] const std::string &get_name(void) const;

        [[nodiscard]] bool has_gamepad(void) const;

        void attach_gamepad(GamepadId id);

        void attach_first_free_gamepad(void);

        void detach_gamepad(void);

        void notify_gamepad_disconnected(void);

        void unbind_action(const std::string &action);

        [[nodiscard]] std::vector<std::string> get_keyboard_key_bindings(KeyboardScancode key) const;

        [[nodiscard]] std::vector<KeyboardScancode> get_keyboard_action_bindings(const std::string &action) const;

        void bind_keyboard_key(KeyboardScancode key, const std::string &action);

        void unbind_keyboard_key(KeyboardScancode key);

        void unbind_keyboard_key(KeyboardScancode key, const std::string &action);

        void bind_mouse_button(MouseButton button, const std::string &action);

        void unbind_mouse_button(MouseButton button);

        void unbind_mouse_button(MouseButton button, const std::string &action);

        void bind_mouse_axis(MouseAxis axis, const std::string &action);

        void unbind_mouse_axis(MouseAxis axis);

        void unbind_mouse_axis(MouseAxis axis, const std::string &action);

        void bind_gamepad_button(GamepadButton button, const std::string &action);

        void unbind_gamepad_button(GamepadButton button);

        void unbind_gamepad_button(GamepadButton button, const std::string &action);

        bool is_action_pressed(const std::string &action);

        double get_action_axis(const std::string &action);

        double get_action_axis_delta(const std::string &action);
    };
}

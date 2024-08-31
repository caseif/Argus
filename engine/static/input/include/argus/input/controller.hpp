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

#include "argus/input/gamepad.hpp"
#include "argus/input/keyboard.hpp"
#include "argus/input/mouse.hpp"

#include <string>
#include <vector>

namespace argus::input {
    // forward declarations
    class InputManager;

    struct pimpl_Controller;

    enum class DeadzoneShape {
        Ellipse,
        Quad,
        Cross,
        MaxValue
    };

    class Controller : AutoCleanupable {
        friend class InputManager;

      private:
        Controller(const std::string &name);

        ~Controller(void) override;

      public:
        pimpl_Controller *m_pimpl;

        Controller(const Controller &) = delete;

        Controller(Controller &&) = delete;

        [[nodiscard]] const std::string &get_name(void) const;

        [[nodiscard]] bool has_gamepad(void) const;

        void attach_gamepad(HidDeviceId id);

        bool attach_first_available_gamepad(void);

        void detach_gamepad(void);

        const char *get_gamepad_name(void) const;

        double get_deadzone_radius(void) const;

        void set_deadzone_radius(double radius);

        void clear_deadzone_radius(void);

        DeadzoneShape get_deadzone_shape(void) const;

        void set_deadzone_shape(DeadzoneShape shape);

        void clear_deadzone_shape(void);

        double get_axis_deadzone_radius(GamepadAxis axis) const;

        void set_axis_deadzone_radius(GamepadAxis axis, double radius);

        void clear_axis_deadzone_radius(GamepadAxis axis);

        DeadzoneShape get_axis_deadzone_shape(GamepadAxis axis) const;

        void set_axis_deadzone_shape(GamepadAxis axis, DeadzoneShape shape);

        void clear_axis_deadzone_shape(GamepadAxis axis);

        void unbind_action(const std::string &action);

        [[nodiscard]] std::vector<std::reference_wrapper<const std::string>> get_keyboard_key_bindings(
                KeyboardScancode key) const;

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

        void bind_gamepad_axis(GamepadAxis axis, const std::string &action);

        void unbind_gamepad_axis(GamepadAxis axis);

        void unbind_gamepad_axis(GamepadAxis axis, const std::string &action);

        bool is_gamepad_button_pressed(GamepadButton button) const;

        double get_gamepad_axis(GamepadAxis axis) const;

        double get_gamepad_axis_delta(GamepadAxis axis) const;

        bool is_action_pressed(const std::string &action) const;

        double get_action_axis(const std::string &action) const;

        double get_action_axis_delta(const std::string &action) const;
    };
}

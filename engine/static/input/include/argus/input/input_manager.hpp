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

#include <string>

namespace argus::input {
    struct pimpl_InputManager;

    class InputManager : AutoCleanupable {
      private:

        InputManager(void);

        ~InputManager(void) override;

      public:
        pimpl_InputManager *m_pimpl;

        InputManager(InputManager &) = delete;

        InputManager(InputManager &&) = delete;

        static InputManager &instance(void);

        Controller &get_controller(const std::string &name);

        Controller &add_controller(const std::string &name);

        void remove_controller(Controller &controller);

        void remove_controller(const std::string &name);

        double get_global_deadzone_radius(void);

        void set_global_deadzone_radius(double radius);

        DeadzoneShape get_global_deadzone_shape(void);

        void set_global_deadzone_shape(DeadzoneShape shape);

        double get_global_axis_deadzone_radius(GamepadAxis axis);

        void set_global_axis_deadzone_radius(GamepadAxis axis, double radius);

        void clear_global_axis_deadzone_radius(GamepadAxis axis);

        DeadzoneShape get_global_axis_deadzone_shape(GamepadAxis axis);

        void set_global_axis_deadzone_shape(GamepadAxis axis, DeadzoneShape shape);

        void clear_global_axis_deadzone_shape(GamepadAxis axis);
    };
}

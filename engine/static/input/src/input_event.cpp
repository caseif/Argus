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

#include "argus/core/event.hpp"

#include "argus/input/controller.hpp"
#include "argus/input/input_event.hpp"

#include <string>

namespace argus::input {
    InputEvent::InputEvent(InputEventType type, const Window &window, const std::string &controller_name,
            const std::string &action, double axis_value, double axis_delta) :
            ArgusEvent(typeid(InputEvent)),
            input_type(type),
            window(window),
            controller_name(controller_name),
            action(action),
            axis_value(axis_value),
            axis_delta(axis_delta) {
    }

    InputEvent::~InputEvent(void) = default;

    const Window &InputEvent::get_window(void) {
        return window;
    }
}

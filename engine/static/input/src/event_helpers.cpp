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

#include "argus/wm/window.hpp"

#include "argus/input/input_event.hpp"
#include "internal/input/event_helpers.hpp"

#include <string>

namespace argus::input {
    void dispatch_button_event(const Window *window, std::string controller_name, std::string action, bool release) {
        auto event_type = release ? InputEventType::ButtonUp : InputEventType::ButtonDown;
        dispatch_event<InputEvent>(event_type, window, std::move(controller_name), std::move(action), 0.0, 0.0);
    }

    void dispatch_axis_event(const Window *window, std::string controller_name, std::string action,
            double value, double delta) {
        dispatch_event<InputEvent>(InputEventType::AxisChanged, window, std::move(controller_name), std::move(action),
                value, delta);
    }
}

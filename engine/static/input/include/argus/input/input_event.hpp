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

#include "argus/core/event.hpp"

#include "argus/input/gamepad.hpp"

#include <string>

namespace argus::input {
    constexpr const char *EVENT_TYPE_INPUT = "input";
    constexpr const char *EVENT_TYPE_INPUT_DEVICE = "input_device";

    enum class InputEventType {
        ButtonDown,
        ButtonUp,
        AxisChanged,
    };

    enum class InputDeviceEventType {
        GamepadConnected,
        GamepadDisconnected,
    };

    struct InputEvent : public ArgusEvent, AutoCleanupable {
        static constexpr const char *get_event_type_id(void) {
            return EVENT_TYPE_INPUT;
        }

        const InputEventType input_type;
        //TODO: replace with std::optional once we have proper binding support for it
        const Window *window;
        const std::string controller_name;
        const std::string action;
        const double axis_value;
        const double axis_delta;

        InputEvent(InputEventType type, const Window *window, std::string controller_name,
                std::string action, double axis_value, double axis_delta);

        InputEvent(const InputEvent &rhs) = delete;

        InputEvent(InputEvent &&rhs) = delete;

        ~InputEvent(void) override;

        const Window *get_window(void);
    };

    struct InputDeviceEvent : ArgusEvent, AutoCleanupable {
        static constexpr const char *get_event_type_id(void) {
            return EVENT_TYPE_INPUT_DEVICE;
        }

        const InputDeviceEventType device_event;
        const std::string controller_name;
        const HidDeviceId device_id;

        InputDeviceEvent(InputDeviceEventType type, std::string controller_name, HidDeviceId device_id);

        InputDeviceEvent(const InputDeviceEvent &) = delete;

        InputDeviceEvent(InputDeviceEvent &&) = delete;

        ~InputDeviceEvent(void) override;
    };
}

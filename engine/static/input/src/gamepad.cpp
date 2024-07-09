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

#include "argus/lowlevel/collections.hpp"
#include "argus/lowlevel/result.hpp"

#include "argus/core/event.hpp"

#include "argus/wm/window.hpp"

#include "argus/input/gamepad.hpp"
#include "argus/input/input_event.hpp"
#include "argus/input/input_manager.hpp"
#include "internal/input/event_helpers.hpp"
#include "internal/input/gamepad.hpp"
#include "internal/input/pimpl/controller.hpp"
#include "internal/input/pimpl/input_manager.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include "SDL_events.h"
#include "SDL_gamecontroller.h"
#include "SDL_joystick.h"
#include "SDL_version.h"

#pragma GCC diagnostic pop

#include <limits>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <cmath>
#include <cstdint>

namespace argus::input {
    typedef uint64_t GamepadButtonState;

    static std::unordered_map<SDL_GameControllerButton, GamepadButton> g_buttons_sdl_to_argus = {
            { SDL_CONTROLLER_BUTTON_INVALID, GamepadButton::Unknown },
            { SDL_CONTROLLER_BUTTON_A, GamepadButton::A },
            { SDL_CONTROLLER_BUTTON_B, GamepadButton::B },
            { SDL_CONTROLLER_BUTTON_X, GamepadButton::X },
            { SDL_CONTROLLER_BUTTON_Y, GamepadButton::Y },
            { SDL_CONTROLLER_BUTTON_DPAD_UP, GamepadButton::DpadUp },
            { SDL_CONTROLLER_BUTTON_DPAD_DOWN, GamepadButton::DpadDown },
            { SDL_CONTROLLER_BUTTON_DPAD_LEFT, GamepadButton::DpadLeft },
            { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, GamepadButton::DpadRight },
            { SDL_CONTROLLER_BUTTON_LEFTSHOULDER, GamepadButton::LBumper },
            { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, GamepadButton::RBumper },
            //{ SDL_CONTROLLER_BUTTON_LTrigger, GamepadButton::LTrigger },
            //{ SDL_CONTROLLER_BUTTON_RTrigger, GamepadButton::RTrigger },
            { SDL_CONTROLLER_BUTTON_LEFTSTICK, GamepadButton::LStick },
            { SDL_CONTROLLER_BUTTON_RIGHTSTICK, GamepadButton::RStick },
            { SDL_CONTROLLER_BUTTON_START, GamepadButton::Start },
            { SDL_CONTROLLER_BUTTON_BACK, GamepadButton::Back },
            { SDL_CONTROLLER_BUTTON_GUIDE, GamepadButton::Guide },
            #if SDL_VERSION_ATLEAST(2, 0, 14)
            { SDL_CONTROLLER_BUTTON_MISC1, GamepadButton::Misc1 },
            { SDL_CONTROLLER_BUTTON_PADDLE1, GamepadButton::L4 },
            { SDL_CONTROLLER_BUTTON_PADDLE2, GamepadButton::R4 },
            { SDL_CONTROLLER_BUTTON_PADDLE3, GamepadButton::L5 },
            { SDL_CONTROLLER_BUTTON_PADDLE4, GamepadButton::R5 },
            #endif
    };

    static std::unordered_map<GamepadButton, SDL_GameControllerButton> g_buttons_argus_to_sdl = {
            { GamepadButton::Unknown, SDL_CONTROLLER_BUTTON_INVALID },
            { GamepadButton::A, SDL_CONTROLLER_BUTTON_A },
            { GamepadButton::B, SDL_CONTROLLER_BUTTON_B },
            { GamepadButton::X, SDL_CONTROLLER_BUTTON_X },
            { GamepadButton::Y, SDL_CONTROLLER_BUTTON_Y },
            { GamepadButton::DpadUp, SDL_CONTROLLER_BUTTON_DPAD_UP },
            { GamepadButton::DpadDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN },
            { GamepadButton::DpadLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT },
            { GamepadButton::DpadRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
            { GamepadButton::LBumper, SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
            { GamepadButton::RBumper, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
            //{ GamepadButton::LTrigger, SDL_CONTROLLER_BUTTON_LTrigger },
            //{ GamepadButton::RTrigger, SDL_CONTROLLER_BUTTON_RTrigger },
            { GamepadButton::LStick, SDL_CONTROLLER_BUTTON_LEFTSTICK },
            { GamepadButton::RStick, SDL_CONTROLLER_BUTTON_RIGHTSTICK },
            { GamepadButton::Start, SDL_CONTROLLER_BUTTON_START },
            { GamepadButton::Back, SDL_CONTROLLER_BUTTON_BACK },
            { GamepadButton::Guide, SDL_CONTROLLER_BUTTON_GUIDE },
            #if SDL_VERSION_ATLEAST(2, 0, 14)
            { GamepadButton::Misc1, SDL_CONTROLLER_BUTTON_MISC1 },
            { GamepadButton::L4, SDL_CONTROLLER_BUTTON_PADDLE1 },
            { GamepadButton::R4, SDL_CONTROLLER_BUTTON_PADDLE2 },
            { GamepadButton::L5, SDL_CONTROLLER_BUTTON_PADDLE3 },
            { GamepadButton::R5, SDL_CONTROLLER_BUTTON_PADDLE4 },
            #endif
    };

    static std::unordered_map<SDL_GameControllerAxis, GamepadAxis> g_axes_sdl_to_argus = {
            { SDL_CONTROLLER_AXIS_INVALID, GamepadAxis::Unknown },
            { SDL_CONTROLLER_AXIS_LEFTX, GamepadAxis::LeftX },
            { SDL_CONTROLLER_AXIS_LEFTY, GamepadAxis::LeftY },
            { SDL_CONTROLLER_AXIS_RIGHTX, GamepadAxis::RightX },
            { SDL_CONTROLLER_AXIS_RIGHTY, GamepadAxis::RightY },
            { SDL_CONTROLLER_AXIS_TRIGGERLEFT, GamepadAxis::LTrigger },
            { SDL_CONTROLLER_AXIS_TRIGGERRIGHT, GamepadAxis::RTrigger },
    };

    static std::unordered_map<GamepadAxis, SDL_GameControllerAxis> g_axes_argus_to_sdl = {
            { GamepadAxis::Unknown, SDL_CONTROLLER_AXIS_INVALID },
            { GamepadAxis::LeftX, SDL_CONTROLLER_AXIS_LEFTX },
            { GamepadAxis::LeftY, SDL_CONTROLLER_AXIS_LEFTY },
            { GamepadAxis::RightX, SDL_CONTROLLER_AXIS_RIGHTX },
            { GamepadAxis::RightY, SDL_CONTROLLER_AXIS_RIGHTY },
            { GamepadAxis::LTrigger, SDL_CONTROLLER_AXIS_TRIGGERLEFT },
            { GamepadAxis::RTrigger, SDL_CONTROLLER_AXIS_TRIGGERRIGHT },
    };

    [[maybe_unused]] static auto g_axis_pairs = make_array<std::pair<GamepadAxis, GamepadAxis>>(
            std::pair { GamepadAxis::LeftX, GamepadAxis::LeftY },
            std::pair { GamepadAxis::RightX, GamepadAxis::RightY },
            std::pair { GamepadAxis::LTrigger, GamepadAxis::RTrigger }
    );

    uint8_t get_connected_gamepad_count(void) {
        return uint8_t(std::min(InputManager::instance().m_pimpl->available_gamepads.size()
                + InputManager::instance().m_pimpl->mapped_gamepads.size(), size_t(UINT8_MAX)));
    }

    uint8_t get_unattached_gamepad_count(void) {
        return uint8_t(std::min(InputManager::instance().m_pimpl->available_gamepads.size(), size_t(UINT8_MAX)));
    }

    std::string get_gamepad_name(HidDeviceId gamepad) {
        auto *controller = SDL_GameControllerFromInstanceID(gamepad);
        if (controller == nullptr) {
            Logger::default_logger().warn("Client queried unknown gamepad ID %d", gamepad);
            return "invalid";
        }

        const char *name = SDL_GameControllerName(controller);
        return name != nullptr ? name : "unknown";
    }

    bool is_gamepad_button_pressed(HidDeviceId gamepad, GamepadButton button) {
        if (std::underlying_type_t<GamepadButton>(button) < 0 || button >= GamepadButton::MaxValue) {
            Logger::default_logger().warn("Client polled invalid gamepad button ordinal %d", button);
            return false;
        }

        auto sdl_button_it = g_buttons_argus_to_sdl.find(button);
        if (sdl_button_it == g_buttons_argus_to_sdl.cend()) {
            Logger::default_logger().warn("Client polled unknown gamepad button ordinal %d", button);
            return false;
        }

        argus_assert(sdl_button_it->second >= 0);

        std::lock_guard<std::mutex> lock(InputManager::instance().m_pimpl->gamepad_states_mutex);

        auto &states = InputManager::instance().m_pimpl->gamepad_states;
        auto it = states.find(gamepad);
        if (it == states.cend()) {
            Logger::default_logger().warn("Client polled unknown gamepad ID %d", gamepad);
            return false;
        }

        auto button_state = it->second.button_state;
        return (button_state & (1 << sdl_button_it->second)) != 0;
    }

    double get_gamepad_axis(HidDeviceId gamepad, GamepadAxis axis) {
        if (std::underlying_type_t<GamepadAxis>(axis) < 0 || axis >= GamepadAxis::MaxValue) {
            Logger::default_logger().warn("Client polled invalid gamepad axis ordinal %d", axis);
            return false;
        }

        std::lock_guard<std::mutex> lock(InputManager::instance().m_pimpl->gamepad_states_mutex);

        auto &states = InputManager::instance().m_pimpl->gamepad_states;
        auto it = states.find(gamepad);
        if (it == states.cend()) {
            Logger::default_logger().warn("Client polled unknown gamepad ID %d", gamepad);
            return false;
        }

        auto axis_state = it->second.axis_state;
        argus_assert(size_t(axis) < axis_state.size());

        return axis_state[size_t(axis)];
    }

    double get_gamepad_axis_delta(HidDeviceId gamepad, GamepadAxis axis) {
        if (std::underlying_type_t<GamepadAxis>(axis) < 0 || axis >= GamepadAxis::MaxValue) {
            Logger::default_logger().warn("Client polled invalid gamepad axis ordinal %d", axis);
            return false;
        }

        std::lock_guard<std::mutex> lock(InputManager::instance().m_pimpl->gamepad_states_mutex);

        auto &states = InputManager::instance().m_pimpl->gamepad_states;
        auto it = states.find(gamepad);
        if (it == states.cend()) {
            Logger::default_logger().warn("Client polled unknown gamepad ID %d", gamepad);
            return false;
        }

        auto deltas = it->second.axis_deltas;
        argus_assert(size_t(axis) < deltas.size());

        return deltas[size_t(axis)];
    }

    static void _init_gamepads(void) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.m_pimpl->gamepads_mutex);

        int joystick_count = SDL_NumJoysticks();

        for (int i = 0; i < joystick_count; i++) {
            const char *name = SDL_JoystickNameForIndex(i);
            if (SDL_IsGameController(i)) {
                Logger::default_logger().debug("Opening joystick '%s' as a gamepad", name);
                SDL_GameControllerOpen(i);
                auto instance_id = SDL_JoystickGetDeviceInstanceID(i);
                if (instance_id == -1) {
                    Logger::default_logger().warn("Unable to get instance ID for joystick at index %d", i);
                    continue;
                }
                manager.m_pimpl->available_gamepads.push_back(SDL_JoystickGetDeviceInstanceID(i));
            } else {
                Logger::default_logger().debug("Joystick '%s' is not reported as a gamepad, ignoring", name);
            }
        }

        auto gamepad_count = manager.m_pimpl->available_gamepads.size();
        if (gamepad_count > 0) {
            Logger::default_logger().info("%zu connected gamepad%s found",
                    gamepad_count, gamepad_count != 1 ? "s" : "");
        } else {
            Logger::default_logger().info("No gamepads connected");
            return;
        }
    }

    static double _normalize_axis(int16_t val) {
        return val == 0
                ? 0
                : val >= 0
                        ? double(val) / std::numeric_limits<int16_t>::max()
                        : -double(val) / std::numeric_limits<int16_t>::min();
    }

    static void _poll_gamepad(HidDeviceId id) {
        auto *gamepad = SDL_GameControllerFromInstanceID(id);
        if (gamepad == nullptr) {
            Logger::default_logger().warn("Failed to get SDL controller from instance ID %d", id);
            return;
        }

        std::optional<Controller *> controller;
        auto controller_name_it = InputManager::instance().m_pimpl->mapped_gamepads.find(id);
        if (controller_name_it != InputManager::instance().m_pimpl->mapped_gamepads.cend()) {
            auto controller_it = InputManager::instance().m_pimpl->controllers.find(controller_name_it->second);
            if (controller_it != InputManager::instance().m_pimpl->controllers.cend()) {
                controller = controller_it->second;
            }
        }

        GamepadButtonState new_button_state = 0;
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
            new_button_state |= uint64_t(SDL_GameControllerGetButton(gamepad, SDL_GameControllerButton(i))) << i;
        }

        std::array<double, size_t(GamepadAxis::MaxValue)> new_axis_state {};
        for (size_t i = 0; i < int(GamepadAxis::MaxValue); i++) {
            new_axis_state[i] = _normalize_axis(
                    SDL_GameControllerGetAxis(gamepad, g_axes_argus_to_sdl[GamepadAxis(i)]));
        }

        for (const auto &[axis_1, axis_2] : g_axis_pairs) {
            DeadzoneShape shape;
            double radius_x;
            double radius_y;
            if (controller.has_value()) {
                shape = controller.value()->get_axis_deadzone_shape(axis_1);
                radius_x = controller.value()->get_axis_deadzone_radius(axis_1);
                radius_y = controller.value()->get_axis_deadzone_radius(axis_2);
            } else {
                shape = InputManager::instance().get_global_axis_deadzone_shape(axis_1);
                radius_x = InputManager::instance().get_global_axis_deadzone_radius(axis_1);
                radius_y = InputManager::instance().get_global_axis_deadzone_radius(axis_2);
            }

            if (radius_x == 0.0 || radius_y == 0.0) {
                continue;
            }

            auto x = new_axis_state[size_t(axis_1)];
            auto y = new_axis_state[size_t(axis_2)];

            auto x2 = std::pow(x, 2);
            auto y2 = std::pow(y, 2);

            // distance from the origin to the bounding box along angle theta (where theta = atan2(x, y))
            double d_boundary = std::abs(x) < std::abs(y)
                    ? std::sqrt(1.0 + x2 / (x2 + y2))
                    : std::abs(x) == std::abs(y)
                            ? std::sqrt(2.0) // degenerate case where we use distance to corner
                            : std::sqrt(1.0 + y2 / (x2 + y2));

            // distance from the origin to the point
            double d_center = std::sqrt(x2 + y2);

            // distance from the origin to the edge of the deadzone at angle theta
            double r_deadzone;

            double new_x;
            double new_y;
            switch (shape) {
                case DeadzoneShape::Ellipse: {
                    auto a2 = std::pow(radius_x, 2);
                    auto b2 = std::pow(radius_y, 2);
                    if (x < radius_x
                            && y < radius_y
                            && x2 / a2 + y2 / b2 <= 1) {
                        new_x = 0;
                        new_y = 0;
                    } else {
                        if (std::abs(std::abs(radius_x) - std::abs(radius_y))
                                <= std::numeric_limits<double>::epsilon()) {
                            // it's a circle so literally just use the constant radius
                            r_deadzone = radius_x;
                        } else {
                            // "radius" of ellipse changes with theta so we need to compute it
                            r_deadzone = std::sqrt(std::abs(2.0 * a2 * b2 - a2 * y2 - b2 * x2));
                        }

                        auto d_deadzone_to_point = d_center - r_deadzone;
                        auto d_deadzone_to_boundary = d_boundary - r_deadzone;

                        argus_assert(d_deadzone_to_boundary > 0.0);
                        new_x = x * (d_deadzone_to_point / d_deadzone_to_boundary);
                        new_y = y * (d_deadzone_to_point / d_deadzone_to_boundary);
                    }
                    break;
                }
                case DeadzoneShape::Quad: {
                    if (std::abs(x) < radius_x
                            && std::abs(y) < radius_y) {
                        new_x = 0;
                        new_y = 0;
                    } else {
                        argus_assert(radius_x < 1.0);
                        argus_assert(radius_y < 1.0);
                        auto r = std::max(std::abs(x), std::abs(y));
                        new_x = x * (r - radius_x) / (1.0 - radius_x);
                        new_y = y * (r - radius_y) / (1.0 - radius_y);
                    }
                    break;
                }
                case DeadzoneShape::Cross: {
                    if (std::abs(x) < radius_x) {
                        new_x = 0;
                    } else {
                        argus_assert(radius_x < 1.0);
                        new_x = x * (std::abs(x) - radius_x) / (1.0 - radius_x);
                    }
                    if (std::abs(y) < radius_y) {
                        new_y = 0;
                    } else {
                        argus_assert(radius_y < 1.0);
                        new_y = y * (std::abs(y) - radius_y) / (1.0 - radius_y);
                    }
                    break;
                }
                default: {
                    Logger::default_logger().debug("Ignoring unknown deadzone shape ordinal %d", shape);
                    continue;
                }
            }

            new_axis_state[size_t(axis_1)] = new_x;
            new_axis_state[size_t(axis_2)] = new_y;
        }

        std::lock_guard<std::mutex> lock(InputManager::instance().m_pimpl->gamepad_states_mutex);

        auto prev_axis_state = InputManager::instance().m_pimpl->gamepad_states[id].axis_state;

        auto &state = InputManager::instance().m_pimpl->gamepad_states[id];

        state.button_state = uint64_t(new_button_state);
        state.axis_state = new_axis_state;
        for (size_t i = 0; i < state.axis_state.size(); i++) {
            state.axis_deltas[i] += new_axis_state[i] - prev_axis_state[i];
        }
    }

    static void _dispatch_button_events(GamepadButton key, bool release) {
        for (auto &[controller_index, controller] : InputManager::instance().m_pimpl->controllers) {
            auto it = controller->m_pimpl->gamepad_button_to_action_bindings.find(key);
            if (it == controller->m_pimpl->gamepad_button_to_action_bindings.end()) {
                continue;
            }

            for (auto &action : it->second) {
                dispatch_button_event(nullptr, controller_index, action, release);
            }
        }
    }

    static void _dispatch_axis_events(GamepadAxis axis, double val, double delta) {
        for (auto &[controller_index, controller] : InputManager::instance().m_pimpl->controllers) {
            auto it = controller->m_pimpl->gamepad_axis_to_action_bindings.find(axis);
            if (it != controller->m_pimpl->gamepad_axis_to_action_bindings.end()) {
                for (auto &action : it->second) {
                    dispatch_axis_event(nullptr, controller_index, action, val, delta);
                }
            }
        }
    }

    static void _dispatch_gamepad_connect_event(HidDeviceId gamepad_id) {
        dispatch_event<InputDeviceEvent>(InputDeviceEventType::GamepadConnected, "", gamepad_id);
    }

    static void _dispatch_gamepad_disconnect_event(std::string controller_name,
            HidDeviceId gamepad_id) {
        dispatch_event<InputDeviceEvent>(InputDeviceEventType::GamepadDisconnected, std::move(controller_name),
                gamepad_id);
    }

    static void _handle_gamepad_events(void) {
        constexpr size_t event_buf_size = 8;
        SDL_Event events[event_buf_size];

        int to_process;
        while ((to_process = SDL_PeepEvents(events, event_buf_size,
                SDL_GETEVENT, SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERDEVICEREMOVED)) > 0) {
            std::lock_guard<std::recursive_mutex> lock(InputManager::instance().m_pimpl->gamepads_mutex);

            for (int i = 0; i < to_process; i++) {
                auto &event = events[i];

                switch (event.type) {
                    case SDL_CONTROLLERBUTTONDOWN:
                    case SDL_CONTROLLERBUTTONUP: {
                        auto it = g_buttons_sdl_to_argus.find(SDL_GameControllerButton(event.cbutton.button));
                        if (it == g_buttons_sdl_to_argus.cend()) {
                            Logger::default_logger().warn("Ignoring event for unknown gamepad button ordinal %d",
                                    event.cbutton.button);
                            break;
                        }
                        _dispatch_button_events(it->second, event.type == SDL_CONTROLLERBUTTONUP);

                        break;
                    }
                    case SDL_CONTROLLERAXISMOTION: {
                        auto it = g_axes_sdl_to_argus.find(SDL_GameControllerAxis(event.caxis.axis));
                        if (it == g_axes_sdl_to_argus.cend()) {
                            Logger::default_logger().warn("Ignoring event for unknown gamepad button ordinal %d",
                                    event.cbutton.button);
                            break;
                        }
                        //TODO: figure out what to do about the delta
                        _dispatch_axis_events(it->second, _normalize_axis(event.caxis.value), 0);

                        break;
                    }
                    case SDL_CONTROLLERDEVICEADDED: {
                        auto device_index = event.cdevice.which;

                        auto *gamepad = SDL_GameControllerOpen(device_index);

                        auto instance_id = SDL_JoystickGetDeviceInstanceID(device_index);
                        if (instance_id < 0) {
                            Logger::default_logger().warn(
                                    "Failed to get device instance ID of newly connected gamepad: %s",
                                    SDL_GetError());
                            continue;
                        }

                        // and they say Java is verbose
                        if (InputManager::instance().m_pimpl->mapped_gamepads.find(instance_id)
                                != InputManager::instance().m_pimpl->mapped_gamepads.end()
                                || std::find(InputManager::instance().m_pimpl->available_gamepads.cbegin(),
                                        InputManager::instance().m_pimpl->available_gamepads.cend(), instance_id)
                                        != InputManager::instance().m_pimpl->available_gamepads.end()) {
                            Logger::default_logger().debug("Ignoring connect event for previously opened gamepad "
                                                           "with instance ID %d", instance_id);
                            // this just decrements the ref count
                            SDL_GameControllerClose(gamepad);
                            continue;
                        }

                        InputManager::instance().m_pimpl->available_gamepads.push_back(instance_id);

                        auto *name = SDL_GameControllerName(gamepad);
                        Logger::default_logger().info("Gamepad '%s' with instance ID %d was connected", name,
                                instance_id);

                        _dispatch_gamepad_connect_event(instance_id);

                        break;
                    }
                    case SDL_CONTROLLERDEVICEREMOVED: {
                        auto instance_id = event.cdevice.which;

                        auto &mapped_gamepads = InputManager::instance().m_pimpl->mapped_gamepads;
                        auto mapped_it = mapped_gamepads.find(instance_id);
                        if (mapped_it != mapped_gamepads.cend()) {
                            auto ctrl_it = InputManager::instance().m_pimpl->controllers.find(mapped_it->second);
                            auto &controllers = InputManager::instance().m_pimpl->controllers;
                            if (ctrl_it != controllers.cend()) {
                                Logger::default_logger().info("Gamepad attached to controller '%s' was disconnected",
                                        ctrl_it->second->get_name().c_str());
                                ctrl_it->second->m_pimpl->was_gamepad_disconnected = true;

                                _dispatch_gamepad_disconnect_event(ctrl_it->second->get_name(), instance_id);
                            } else {
                                // shouldn't happen

                                mapped_gamepads.erase(mapped_it);

                                _dispatch_gamepad_disconnect_event("", instance_id);
                            }
                        } else {
                            auto &avail_gamepads = InputManager::instance().m_pimpl->available_gamepads;
                            auto avail_it = std::find(avail_gamepads.cbegin(), avail_gamepads.cend(),
                                    event.cdevice.which);
                            if (avail_it != avail_gamepads.cend()) {
                                avail_gamepads.erase(avail_it);
                            }

                            Logger::default_logger().info("Gamepad with instance ID %d was disconnected", instance_id);

                            _dispatch_gamepad_disconnect_event("", instance_id);
                        }

                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    void update_gamepads(void) {
        auto &manager = InputManager::instance();

        if (!manager.m_pimpl->are_gamepads_initted) {
            _init_gamepads();

            manager.m_pimpl->are_gamepads_initted = true;
        }

        _handle_gamepad_events();

        for (auto gamepad_id : manager.m_pimpl->available_gamepads) {
            _poll_gamepad(gamepad_id);
        }

        for (auto &[gamepad_id, _] : manager.m_pimpl->mapped_gamepads) {
            _poll_gamepad(gamepad_id);
        }
    }

    void flush_gamepad_deltas(void) {
        std::lock_guard<std::mutex> lock(InputManager::instance().m_pimpl->gamepad_states_mutex);

        for (auto &[_, gamepad] : InputManager::instance().m_pimpl->gamepad_states) {
            gamepad.axis_deltas = {};
        }
    }

    Result<void, std::string> assoc_gamepad(HidDeviceId id, const std::string &controller_name) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.m_pimpl->gamepads_mutex);

        auto &gamepads = manager.m_pimpl->available_gamepads;
        auto it = std::find(gamepads.cbegin(), gamepads.cend(), id);
        if (it == manager.m_pimpl->available_gamepads.cend()) {
            return err<void, std::string>("Gamepad ID is not valid or is already in use");
        }

        manager.m_pimpl->available_gamepads.erase(it);
        manager.m_pimpl->mapped_gamepads.insert({ id, controller_name });

        return ok<void, std::string>();
    }

    Result<HidDeviceId, std::string> assoc_first_available_gamepad(const std::string &controller_name) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.m_pimpl->gamepads_mutex);

        if (manager.m_pimpl->available_gamepads.empty()) {
            return err<HidDeviceId, std::string>("No available gamepads");
        }

        auto front = manager.m_pimpl->available_gamepads.front();
        return assoc_gamepad(front, controller_name)
                .and_then<HidDeviceId>([front](void) { return ok<int, std::string>(front); });
    }

    void unassoc_gamepad(HidDeviceId id) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.m_pimpl->gamepads_mutex);

        auto it = manager.m_pimpl->mapped_gamepads.find(id);

        if (it == manager.m_pimpl->mapped_gamepads.cend()) {
            Logger::default_logger().warn("Client attempted to close unmapped gamepad instance ID %d", id);
            return;
        }

        manager.m_pimpl->mapped_gamepads.erase(it);
        manager.m_pimpl->available_gamepads.push_back(id);
    }

    static void _close_gamepad(HidDeviceId id) {
        auto *controller = SDL_GameControllerFromInstanceID(id);
        if (controller == nullptr) {
            Logger::default_logger().warn("Failed to get SDL gamepad with instance ID %d while "
                                          "deinitializing gamepads", id);
            return;
        }
        SDL_GameControllerClose(controller);
    }

    void deinit_gamepads(void) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.m_pimpl->gamepads_mutex);

        for (auto id : manager.m_pimpl->available_gamepads) {
            _close_gamepad(id);
        }

        for (const auto &kv : manager.m_pimpl->mapped_gamepads) {
            _close_gamepad(kv.first);
        }
    }
}

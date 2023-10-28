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

#include "argus/lowlevel/macros.hpp"

#include "argus/input/gamepad.hpp"
#include "argus/input/input_manager.hpp"
#include "internal/input/gamepad.hpp"
#include "internal/input/pimpl/controller.hpp"
#include "internal/input/pimpl/input_manager.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "SDL_events.h"
#pragma GCC diagnostic pop
#include "SDL_gamecontroller.h"
#include "SDL_joystick.h"

#include <map>

namespace argus::input {
    typedef uint64_t GamepadButtonState;

    static std::map<SDL_GameControllerButton, GamepadButton> g_buttons_sdl_to_argus = {
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
            { SDL_CONTROLLER_BUTTON_PADDLE1, GamepadButton::L4 },
            { SDL_CONTROLLER_BUTTON_PADDLE2, GamepadButton::R4 },
            { SDL_CONTROLLER_BUTTON_PADDLE3, GamepadButton::L5 },
            { SDL_CONTROLLER_BUTTON_PADDLE4, GamepadButton::R5 },
            { SDL_CONTROLLER_BUTTON_START, GamepadButton::Start },
            { SDL_CONTROLLER_BUTTON_BACK, GamepadButton::Back },
            { SDL_CONTROLLER_BUTTON_GUIDE, GamepadButton::Guide },
            { SDL_CONTROLLER_BUTTON_MISC1, GamepadButton::Misc1 },
    };

    static std::map<GamepadButton, SDL_GameControllerButton> g_buttons_argus_to_sdl = {
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
            { GamepadButton::L4, SDL_CONTROLLER_BUTTON_PADDLE1 },
            { GamepadButton::R4, SDL_CONTROLLER_BUTTON_PADDLE2 },
            { GamepadButton::L5, SDL_CONTROLLER_BUTTON_PADDLE3 },
            { GamepadButton::R5, SDL_CONTROLLER_BUTTON_PADDLE4 },
            { GamepadButton::Start, SDL_CONTROLLER_BUTTON_START },
            { GamepadButton::Back, SDL_CONTROLLER_BUTTON_BACK },
            { GamepadButton::Guide, SDL_CONTROLLER_BUTTON_GUIDE },
            { GamepadButton::Misc1, SDL_CONTROLLER_BUTTON_MISC1 },
    };

    static std::map<SDL_GameControllerAxis, GamepadAxis> g_axes_sdl_to_argus = {
            { SDL_CONTROLLER_AXIS_INVALID, GamepadAxis::Unknown },
            { SDL_CONTROLLER_AXIS_LEFTX, GamepadAxis::LeftX },
            { SDL_CONTROLLER_AXIS_LEFTY, GamepadAxis::LeftY },
            { SDL_CONTROLLER_AXIS_RIGHTX, GamepadAxis::RightX },
            { SDL_CONTROLLER_AXIS_RIGHTY, GamepadAxis::RightY },
            { SDL_CONTROLLER_AXIS_TRIGGERLEFT, GamepadAxis::LTrigger },
            { SDL_CONTROLLER_AXIS_TRIGGERRIGHT, GamepadAxis::RTrigger },
    };

    static std::map<GamepadAxis, SDL_GameControllerAxis> g_axes_argus_to_sdl = {
            { GamepadAxis::Unknown, SDL_CONTROLLER_AXIS_INVALID },
            { GamepadAxis::LeftX, SDL_CONTROLLER_AXIS_LEFTX },
            { GamepadAxis::LeftY, SDL_CONTROLLER_AXIS_LEFTY },
            { GamepadAxis::RightX, SDL_CONTROLLER_AXIS_RIGHTX },
            { GamepadAxis::RightY, SDL_CONTROLLER_AXIS_RIGHTY },
            { GamepadAxis::LTrigger, SDL_CONTROLLER_AXIS_TRIGGERLEFT },
            { GamepadAxis::RTrigger, SDL_CONTROLLER_AXIS_TRIGGERRIGHT },
    };

    uint8_t get_connected_gamepad_count(void) {
        return uint8_t(std::min(InputManager::instance().pimpl->available_gamepads.size()
                + InputManager::instance().pimpl->mapped_gamepads.size(), size_t(UINT8_MAX)));
    }

    uint8_t get_unattached_gamepad_count(void) {
        return uint8_t(std::min(InputManager::instance().pimpl->available_gamepads.size(), size_t(UINT8_MAX)));
    }

    std::string get_gamepad_name(GamepadId id) {
        auto *controller = SDL_GameControllerFromInstanceID(id);
        if (controller == nullptr) {
            Logger::default_logger().warn("Client queried unknown gamepad ID %d", id);
        }

        const char *name = SDL_GameControllerName(controller);
        return name != nullptr ? name : "unknown";
    }

    bool is_gamepad_button_pressed(GamepadId id, GamepadButton button) {
        if (button == GamepadButton::Unknown) {
            Logger::default_logger().warn("Client polled invalid gamepad button ordinal %d", button);
        }

        auto &states = InputManager::instance().pimpl->gamepad_button_states;
        auto it = states.find(id);
        if (it == states.cend()) {
            Logger::default_logger().warn("Client polled unknown gamepad ID %d", id);
            return false;
        }

        auto sdl_button_it = g_buttons_argus_to_sdl.find(button);
        if (sdl_button_it == g_buttons_argus_to_sdl.cend()) {
            Logger::default_logger().warn("Client polled unknown gamepad button ordinal %d", button);
            return false;
        }

        assert(sdl_button_it->second >= 0);

        return (it->second.load() & (1 << sdl_button_it->second)) != 0;
    }

    static void _init_gamepads(void) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.pimpl->gamepads_mutex);

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
                manager.pimpl->available_gamepads.push_back(SDL_JoystickGetDeviceInstanceID(i));
            } else {
                Logger::default_logger().debug("Joystick '%s' is not reported as a gamepad, ignoring", name);
            }
        }

        auto gamepad_count = manager.pimpl->available_gamepads.size();
        if (gamepad_count > 0) {
            Logger::default_logger().info("%zu connected gamepad%s found",
                    gamepad_count, gamepad_count != 1 ? "s" : "");
        } else {
            Logger::default_logger().info("No gamepads connected");
            return;
        }
    }

    [[maybe_unused]] static void _update_gamepad(GamepadId id) {
        auto *controller = SDL_GameControllerFromInstanceID(id);
        if (controller == nullptr) {
            Logger::default_logger().warn("Failed to get SDL controller from instance ID %d", id);
            return;
        }

        GamepadButtonState state = 0;
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
            state |= uint64_t(SDL_GameControllerGetButton(controller, SDL_GameControllerButton(i))) << i;
        }

        InputManager::instance().pimpl->gamepad_button_states[id] = int64_t(state);
    }

    void update_gamepads(void) {
        auto &manager = InputManager::instance();

        if (!manager.pimpl->are_gamepads_initted) {
            _init_gamepads();

            manager.pimpl->are_gamepads_initted = true;
        }

        for (auto gamepad_id : manager.pimpl->available_gamepads) {
            _update_gamepad(gamepad_id);
        }

        for (auto &gamepad_kv : manager.pimpl->mapped_gamepads) {
            _update_gamepad(gamepad_kv.first);
        }
    }

    void handle_gamepad_events(void) {
        constexpr size_t event_buf_size = 8;
        SDL_Event events[event_buf_size];

        int to_process;
        while ((to_process = SDL_PeepEvents(events, event_buf_size,
                SDL_GETEVENT, SDL_CONTROLLERDEVICEADDED, SDL_CONTROLLERDEVICEREMOVED)) > 0) {
            std::lock_guard<std::recursive_mutex> lock(InputManager::instance().pimpl->gamepads_mutex);

            for (int i = 0; i < to_process; i++) {
                auto &event = events[i];

                if (event.type == SDL_CONTROLLERDEVICEADDED) {
                    auto instance_id = SDL_JoystickGetDeviceInstanceID(event.cdevice.which);
                    InputManager::instance().pimpl->available_gamepads.push_back(instance_id);
                } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                    auto &mapped_gamepads = InputManager::instance().pimpl->mapped_gamepads;
                    auto mapped_it = mapped_gamepads.find(event.cdevice.which);
                    if (mapped_it != mapped_gamepads.cend()) {
                        auto ctrl_it = InputManager::instance().pimpl->controllers.find(mapped_it->second);
                        auto &controllers = InputManager::instance().pimpl->controllers;
                        if (ctrl_it != controllers.cend()) {
                            Logger::default_logger().warn("Gamepad attached to controller '%s' was disconnected",
                                    ctrl_it->second->get_name().c_str());
                            ctrl_it->second->detach_gamepad();
                        } else {
                            mapped_gamepads.erase(mapped_it);
                        }
                    } else {
                        auto &avail_gamepads = InputManager::instance().pimpl->available_gamepads;
                        auto avail_it = std::find(avail_gamepads.cbegin(), avail_gamepads.cend(), event.cdevice.which);
                        if (avail_it != avail_gamepads.cend()) {
                            avail_gamepads.erase(avail_it);
                        }
                    }
                }
            }
        }
    }

    void assoc_gamepad(GamepadId id, const std::string &controller_name) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.pimpl->gamepads_mutex);

        auto &gamepads = manager.pimpl->available_gamepads;
        auto it = std::find(gamepads.cbegin(), gamepads.cend(), id);
        if (it == manager.pimpl->available_gamepads.cend()) {
            throw std::invalid_argument("Gamepad ID is not valid or is already in use");
        }

        manager.pimpl->available_gamepads.erase(it);
        manager.pimpl->mapped_gamepads.insert({ id, controller_name });
    }

    GamepadId assoc_first_available_gamepad(const std::string &controller_name) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.pimpl->gamepads_mutex);

        if (manager.pimpl->available_gamepads.empty()) {
            throw std::runtime_error("No gamepads are available to attach to controller");
        }

        auto front = manager.pimpl->available_gamepads.front();
        assoc_gamepad(front, controller_name);

        return front;
    }

    void unassoc_gamepad(GamepadId id) {
        auto &manager = InputManager::instance();

        std::lock_guard<std::recursive_mutex> lock(manager.pimpl->gamepads_mutex);

        auto it = manager.pimpl->mapped_gamepads.find(id);

        if (it == manager.pimpl->mapped_gamepads.cend()) {
            Logger::default_logger().warn("Client attempted to close unmapped gamepad instance ID %d", id);
            return;
        }

        manager.pimpl->mapped_gamepads.erase(it);
        manager.pimpl->available_gamepads.push_back(id);
    }

    static void _close_gamepad(GamepadId id) {
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

        std::lock_guard<std::recursive_mutex> lock(manager.pimpl->gamepads_mutex);

        for (auto id : manager.pimpl->available_gamepads) {
            _close_gamepad(id);
        }

        for (const auto &kv : manager.pimpl->mapped_gamepads) {
            _close_gamepad(kv.first);
        }
    }
}

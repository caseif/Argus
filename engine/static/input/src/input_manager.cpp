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

#include "argus/lowlevel/memory.hpp"

#include "argus/input/controller.hpp"
#include "argus/input/input_event.hpp"
#include "argus/input/input_manager.hpp"
#include "internal/input/defines.hpp"
#include "internal/input/input_manager.hpp"
#include "internal/input/pimpl/controller.hpp"
#include "internal/input/pimpl/input_manager.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <vector>

namespace argus::input {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_InputManager));

    InputManager &InputManager::instance(void) {
        static InputManager instance;
        return instance;
    }

    InputManager::InputManager(void) :
            pimpl(&g_pimpl_pool.construct<pimpl_InputManager>()) {
    }

    InputManager::~InputManager(void) {
        if (pimpl != nullptr) {
            std::vector<std::string> remove_names;
            for (const auto &pair : pimpl->controllers) {
                delete pair.second;
            }

            g_pimpl_pool.destroy(pimpl);
        }
    }

    Controller &InputManager::get_controller(const std::string &name) {
        auto res = std::find_if(pimpl->controllers.begin(), pimpl->controllers.end(),
                [name](auto &pair) { return pair.second->get_name() == name; });

        if (res == pimpl->controllers.end()) {
            throw std::invalid_argument("Invalid controller index");
        }

        return *res->second;
    }

    Controller &InputManager::add_controller(const std::string &name, bool assign_gamepad) {
        if (pimpl->controllers.size() >= MAX_CONTROLLERS) {
            throw std::invalid_argument("Controller limit reached");
        }

        auto controller = new Controller(name, assign_gamepad);

        pimpl->controllers.insert({ name, controller });

        return *controller;
    }

    void InputManager::remove_controller(Controller &controller) {
        remove_controller(controller.get_name());
    }

    void InputManager::remove_controller(const std::string &name) {
        auto res = pimpl->controllers.find(name);
        if (res == pimpl->controllers.end()) {
            throw std::invalid_argument("Client attempted to remove unknown controller index");
        }

        delete res->second;

        pimpl->controllers.erase(res);
    }

    static void _dispatch_button_event(const Window &window, const std::string &controller_name, std::string &action,
            bool release) {
        auto event_type = release ? InputEventType::ButtonUp : InputEventType::ButtonDown;
        dispatch_event<InputEvent>(event_type, window, controller_name, action, 0.0, 0.0);
    }

    static void _dispatch_axis_event(const Window &window, const std::string &controller_name, std::string &action,
            double value, double delta) {
        dispatch_event<InputEvent>(InputEventType::AxisChanged, window, controller_name, action, value, delta);
    }

    void InputManager::handle_key_press(const Window &window, KeyboardScancode key, bool release) const {
        //TODO: ignore while in a TextInputContext once we properly implement that

        for (auto &pair : pimpl->controllers) {
            auto controller_index = pair.first;
            auto &controller = *pair.second;

            auto it = controller.pimpl->key_to_action_bindings.find(key);
            if (it == controller.pimpl->key_to_action_bindings.end()) {
                continue;
            }

            for (auto &action : it->second) {
                _dispatch_button_event(window, controller_index, action, release);
            }
        }
    }

    void InputManager::handle_mouse_button_press(const Window &window, MouseButton button, bool release) const {
        for (auto &pair : pimpl->controllers) {
            auto controller_index = pair.first;
            auto &controller = *pair.second;

            auto it = controller.pimpl->mouse_button_to_action_bindings.find(button);
            if (it == controller.pimpl->mouse_button_to_action_bindings.end()) {
                continue;
            }

            for (auto &action : it->second) {
                _dispatch_button_event(window, controller_index, action, release);
            }
        }
    }

    void InputManager::handle_mouse_axis_change(const Window &window, MouseAxis axis, double value,
            double delta) const {
        for (auto &pair : pimpl->controllers) {
            auto controller_index = pair.first;
            auto &controller = *pair.second;

            auto it = controller.pimpl->mouse_axis_to_action_bindings.find(axis);
            if (it == controller.pimpl->mouse_axis_to_action_bindings.end()) {
                continue;
            }

            for (auto &action : it->second) {
                _dispatch_axis_event(window, controller_index, action, value, delta);
            }
        }
    }

    static std::vector<GamepadId> _get_all_gamepads(void) {
        std::vector<GamepadId> gamepads;
        int joystick_count = SDL_NumJoysticks();
        for (int i = 0; i < joystick_count; i++) {
            if (SDL_IsGameController(i)) {
                gamepads.push_back(SDL_JoystickGetDeviceInstanceID(i));
            }
        }
        return gamepads;
    }

    void update_input_manager(InputManager &manager) {
        UNUSED(manager);
        if (!manager.pimpl->are_gamepads_initted) {
            std::lock_guard<std::mutex> lock(manager.pimpl->gamepads_mutex);
            manager.pimpl->available_gamepads = _get_all_gamepads();
            manager.pimpl->are_gamepads_initted = true;
        }
    }
}

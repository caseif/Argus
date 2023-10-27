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
#include "argus/lowlevel/vector.hpp"

#include "argus/input/controller.hpp"
#include "argus/input/input_manager.hpp"
#include "argus/input/keyboard.hpp"
#include "argus/input/mouse.hpp"
#include "internal/input/pimpl/controller.hpp"
#include "internal/input/pimpl/input_manager.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace argus::input {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Controller));

    Controller::Controller(const std::string &name, bool assign_gamepad) :
            pimpl(&g_pimpl_pool.construct<pimpl_Controller>(name)) {
        if (assign_gamepad) {
            attach_first_free_gamepad();
        }
    }

    Controller::~Controller(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    const std::string &Controller::get_name(void) const {
        return pimpl->name;
    }

    bool Controller::has_gamepad(void) const {
        return pimpl->attached_gamepad.has_value();
    }

    void Controller::attach_gamepad(GamepadId id) {
        if (pimpl->attached_gamepad.has_value()) {
            throw std::invalid_argument("Controller already has associated gamepad");
        }

        auto &gamepads = InputManager::instance().pimpl->available_gamepads;
        auto it = std::find(gamepads.cbegin(), gamepads.cend(), id);
        if (it == InputManager::instance().pimpl->available_gamepads.cend()) {
            throw std::invalid_argument("Gamepad ID is not valid or is already in use");
        }

        pimpl->attached_gamepad = id;
    }

    void Controller::attach_first_free_gamepad(void) {
        if (pimpl->attached_gamepad.has_value()) {
            throw std::invalid_argument("Controller already has associated gamepad");
        }

        if (InputManager::instance().pimpl->available_gamepads.empty()) {
            throw std::runtime_error("No gamepads are available to attach to controller");
        }

        pimpl->attached_gamepad = InputManager::instance().pimpl->available_gamepads.front();
    }

    void Controller::detach_gamepad(void) {
        if (!pimpl->attached_gamepad.has_value()) {
            // silently fail
            return;
        }

        pimpl->attached_gamepad = std::nullopt;
    }

    void Controller::notify_gamepad_disconnected(void) {
        pimpl->was_gamepad_disconnected = true;
    }

    void Controller::unbind_action(const std::string &action) {
        if (pimpl->action_to_key_bindings.find(action) == pimpl->action_to_key_bindings.end()) {
            return;
        }

        // remove action from binding list of keys it's bound to
        for (auto key : pimpl->action_to_key_bindings[action]) {
            remove_from_vector(pimpl->key_to_action_bindings[key], action);
        }

        // remove binding list of action
        pimpl->action_to_key_bindings.erase(action);
    }

    template<typename T>
    static void _bind_thing(std::map<T, std::vector<std::string>> &to_map,
            std::map<std::string, std::vector<T>> &from_map, T thing, const std::string action) {
        // we maintain two binding maps because actions and "things" have a
        // many-to-many relationship; i.e. each key may be bound to multiple
        // actions and each action may have multiple keys bound to it

        // insert into the thing-to-actions map
        auto &tta_vec = to_map[thing];
        if (std::find(tta_vec.begin(), tta_vec.end(), action) == tta_vec.end()) {
            tta_vec.push_back(action);
        }
        // nothing to do if the binding already exists

        // insert into the action-to-things map
        auto &att_vec = from_map[action];
        if (std::find(att_vec.begin(), att_vec.end(), thing) == att_vec.end()) {
            att_vec.push_back(thing);
        }
        // nothing to do if the binding already exists
    }

    template<typename T>
    static void _unbind_thing(std::map<T, std::vector<std::string>> &to_map,
            std::map<std::string, std::vector<T>> &from_map, T thing) {
        if (to_map.find(thing) == to_map.end()) {
            return;
        }

        // remove action from binding list of things it's bound to
        for (const auto &action : to_map[thing]) {
            remove_from_vector(from_map[action], thing);
        }

        // remove binding list of thing
        to_map.erase(thing);
    }

    template<typename T>
    static void _unbind_thing(std::map<T, std::vector<std::string>> &to_map,
            std::map<std::string, std::vector<T>> &from_map, T thing, const std::string &action) {
        auto actions_it = from_map.find(action);
        if (actions_it != from_map.end()) {
            remove_from_vector(actions_it->second, thing);
        }

        auto things_it = to_map.find(thing);
        if (things_it != to_map.end()) {
            remove_from_vector(things_it->second, action);
        }
    }

    std::vector<std::string> Controller::get_keyboard_key_bindings(KeyboardScancode key) const {
        auto it = pimpl->key_to_action_bindings.find(key);
        if (it != pimpl->key_to_action_bindings.end()) {
            return it->second; // implicitly deep-copied
        }

        // no bindings so just return empty vector
        return {};
    }

    std::vector<KeyboardScancode> Controller::get_keyboard_action_bindings(const std::string &action) const {
        auto it = pimpl->action_to_key_bindings.find(action);
        if (it != pimpl->action_to_key_bindings.end()) {
            return it->second; // implicitly deep-copied
        }

        // no bindings so just return empty vector
        return {};
    }

    void Controller::bind_keyboard_key(KeyboardScancode key, const std::string &action) {
        _bind_thing(pimpl->key_to_action_bindings, pimpl->action_to_key_bindings, key, action);
    }

    void Controller::unbind_keyboard_key(KeyboardScancode key) {
        _unbind_thing(pimpl->key_to_action_bindings, pimpl->action_to_key_bindings, key);
    }

    void Controller::unbind_keyboard_key(KeyboardScancode key, const std::string &action) {
        _unbind_thing(pimpl->key_to_action_bindings, pimpl->action_to_key_bindings, key, action);
    }

    void Controller::bind_mouse_button(MouseButton button, const std::string &action) {
        _bind_thing(pimpl->mouse_button_to_action_bindings, pimpl->action_to_mouse_button_bindings, button, action);
    }

    void Controller::unbind_mouse_button(MouseButton button) {
        _unbind_thing(pimpl->mouse_button_to_action_bindings, pimpl->action_to_mouse_button_bindings, button);
    }

    void Controller::unbind_mouse_button(MouseButton button, const std::string &action) {
        _unbind_thing(pimpl->mouse_button_to_action_bindings, pimpl->action_to_mouse_button_bindings, button,
                action);//TODO
    }

    void Controller::bind_mouse_axis(MouseAxis axis, const std::string &action) {
        _bind_thing(pimpl->mouse_axis_to_action_bindings, pimpl->action_to_mouse_axis_bindings, axis, action);
    }

    void Controller::unbind_mouse_axis(MouseAxis axis) {
        _unbind_thing(pimpl->mouse_axis_to_action_bindings, pimpl->action_to_mouse_axis_bindings, axis);
    }

    void Controller::unbind_mouse_axis(MouseAxis axis, const std::string &action) {
        _unbind_thing(pimpl->mouse_axis_to_action_bindings, pimpl->action_to_mouse_axis_bindings, axis, action);
    }

    bool Controller::is_action_pressed(const std::string &action) {
        auto kb_it = pimpl->action_to_key_bindings.find(action);
        if (kb_it != pimpl->action_to_key_bindings.cend()) {
            for (auto key : kb_it->second) {
                if (is_key_pressed(key)) {
                    return true;
                }
            }
        }

        auto mouse_it = pimpl->action_to_mouse_button_bindings.find(action);
        if (mouse_it != pimpl->action_to_mouse_button_bindings.cend()) {
            for (auto btn : mouse_it->second) {
                if (is_mouse_button_pressed(btn)) {
                    return true;
                }
            }
        }

        return false;
    }

    double Controller::get_action_axis(const std::string &action) {
        auto mouse_it = pimpl->action_to_mouse_axis_bindings.find(action);
        if (mouse_it != pimpl->action_to_mouse_axis_bindings.cend() && !mouse_it->second.empty()) {
            return get_mouse_axis(mouse_it->second.front());
        }

        return 0;
    }

    double Controller::get_action_axis_delta(const std::string &action) {
        auto mouse_it = pimpl->action_to_mouse_axis_bindings.find(action);
        if (mouse_it != pimpl->action_to_mouse_axis_bindings.cend() && !mouse_it->second.empty()) {
            return get_mouse_axis_delta(mouse_it->second.front());
        }

        return 0;
    }
}

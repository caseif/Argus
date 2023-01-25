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
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/vector.hpp"

#include "argus/input/controller.hpp"
#include "internal/input/pimpl/controller.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace argus::input {
    static AllocPool g_pimpl_pool(sizeof(pimpl_Controller));

    Controller::Controller(ControllerIndex index) :
            pimpl(&g_pimpl_pool.construct<pimpl_Controller>(index)) {
    }

    Controller::~Controller(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    ControllerIndex Controller::get_index(void) const {
        return pimpl->index;
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
        for (auto action : to_map[thing]) {
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

    const std::vector<std::string> Controller::get_keyboard_key_bindings(KeyboardScancode key) const {
        auto it = pimpl->key_to_action_bindings.find(key);
        if (it != pimpl->key_to_action_bindings.end()) {
            return it->second; // implicitly deep-copied
        }

        // no bindings so just return empty vector
        return {};
    }

    const std::vector<KeyboardScancode> Controller::get_keyboard_action_bindings(const std::string &action) const {
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

    void Controller::bind_mouse_button(MouseButtonIndex button, const std::string &action) {
        _bind_thing(pimpl->mouse_button_to_action_bindings, pimpl->action_to_mouse_button_bindings, button, action);
    }

    void Controller::unbind_mouse_button(MouseButtonIndex button) {
        _unbind_thing(pimpl->mouse_button_to_action_bindings, pimpl->action_to_mouse_button_bindings, button);
    }

    void Controller::unbind_mouse_button(MouseButtonIndex button, const std::string &action) {
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
}

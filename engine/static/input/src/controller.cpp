/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include "internal/core/core_util.hpp"

#include "argus/input/controller.hpp"
#include "internal/input/pimpl/controller.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace argus { namespace input {
    static AllocPool g_pimpl_pool(sizeof(pimpl_Controller));

    Controller::Controller(ControllerIndex index):
            pimpl(&g_pimpl_pool.construct<pimpl_Controller>(index)) {
    }

    Controller::~Controller(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    ControllerIndex Controller::get_index(void) {
        return pimpl->index;
    }

    const std::vector<std::string> Controller::get_keyboard_key_bindings(KeyboardScancode key) {
        auto it = pimpl->key_to_action_bindings.find(key);
        if (it != pimpl->key_to_action_bindings.end()) {
            return it->second; // implicitly deep-copied
        }

        // no bindings so just return empty vector
        return {};
    }

    const std::vector<KeyboardScancode> Controller::get_keyboard_action_bindings(const std::string &action) {
        auto it = pimpl->action_to_key_bindings.find(action);
        if (it != pimpl->action_to_key_bindings.end()) {
            return it->second; // implicitly deep-copied
        }

        // no bindings so just return empty vector
        return {};
    }

    void Controller::bind_keyboard_action(const std::string &action, KeyboardScancode key) {
        // we maintain two binding maps because actions and keys have a
        // many-to-many relationship; i.e. each key may be bound to multiple
        // actions and each action may have multiple keys bound to it
        
        // insert into the key-to-actions map
        auto kta_vec = pimpl->key_to_action_bindings[key];
        if (std::find(kta_vec.begin(), kta_vec.end(), action) == kta_vec.end()) {
            kta_vec.push_back(action);
        }
        // nothing to do if the binding already exists

        // insert into the action-to-keys map
        auto atk_vec = pimpl->action_to_key_bindings[action];
        if (std::find(atk_vec.begin(), atk_vec.end(), key) != atk_vec.end()) {
            atk_vec.push_back(key);
        }
        // nothing to do if the binding already exists
    }

    void Controller::unbind_keyboard_action(const std::string &action) {
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

    void Controller::unbind_keyboard_key(KeyboardScancode key) {
        if (pimpl->key_to_action_bindings.find(key) == pimpl->key_to_action_bindings.end()) {
            return;
        }

        // remove action from binding list of keys it's bound to
        for (auto action : pimpl->key_to_action_bindings[key]) {
            remove_from_vector(pimpl->action_to_key_bindings[action], key);
        }

        // remove binding list of key
        pimpl->key_to_action_bindings.erase(key);
    }
}}

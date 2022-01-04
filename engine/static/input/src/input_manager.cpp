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

#include "argus/input/controller.hpp"
#include "argus/input/input_event.hpp"
#include "argus/input/input_manager.hpp"
#include "internal/input/pimpl/controller.hpp"
#include "internal/input/pimpl/input_manager.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <vector>

namespace argus { namespace input {
    static AllocPool g_pimpl_pool(sizeof(pimpl_InputManager));

    InputManager &InputManager::instance(void) {
        static InputManager instance;
        return instance;
    }

    InputManager::InputManager(void):
            pimpl(&g_pimpl_pool.construct<pimpl_InputManager>()) {
    }

    InputManager::~InputManager(void) {
        if (pimpl != nullptr) {
            std::vector<ControllerIndex> remove_indices;
            for (auto pair : pimpl->controllers) {
                delete pair.second;
            }

            g_pimpl_pool.destroy(pimpl);
        }
    }

    Controller &InputManager::get_controller(ControllerIndex controller_index) {
        auto res = std::find_if(pimpl->controllers.begin(), pimpl->controllers.end(),
                [controller_index](auto &pair) { return pair.second->get_index() == controller_index; });

        if (res == pimpl->controllers.end()) {
            throw std::invalid_argument("Invalid controller index");
        }

        return *res->second;
    }

    Controller &InputManager::add_controller(void) {
        if (pimpl->controllers.size() > UINT16_MAX) {
            throw std::invalid_argument("Controller limit reached");
        }

        ControllerIndex last_index = UINT16_MAX;
        ControllerIndex free_index = 0;
        for (auto &pair : pimpl->controllers) {
            if (last_index == UINT16_MAX) {
                // this is the first index we've inspected
                if (pair.first != 0) {
                    free_index = 0;
                    break;
                }
            } else {
                if (pair.first != last_index + 1) {
                    free_index = last_index + 1;
                    break;
                }

                // should only be possible if all values in range
                // [0, UINT16_MAX] have been checked, which is impossible via
                // the pigeonhole principle because we already verified the map
                // is smaller than UINT16_MAX
                assert(pair.first != UINT16_MAX);
            }

            last_index = pair.first;
        }

        auto controller = new Controller(free_index);

        pimpl->controllers.insert({ free_index, controller });

        return *controller;
    }

    void InputManager::remove_controller(Controller &controller) {
        remove_controller(controller.get_index());
    }

    void InputManager::remove_controller(ControllerIndex controller_index) {
        auto res = pimpl->controllers.find(controller_index);
        if (res == pimpl->controllers.end()) {
            throw std::invalid_argument("Client attempted to remove unknown controller index");
        }

        delete res->second;

        pimpl->controllers.erase(res);
    }

    static void _dispatch_button_event(const Window &window, ControllerIndex controller_index, std::string &action,
            bool release) {
        auto event_type = release ? InputEventType::ButtonUp : InputEventType::ButtonDown;
        dispatch_event<InputEvent>(event_type, window, controller_index, action, 0.0, 0.0);
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
}}

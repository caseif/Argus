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

#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/collections.hpp"

#include "argus/input/controller.hpp"
#include "argus/input/input_manager.hpp"
#include "argus/input/keyboard.hpp"
#include "argus/input/mouse.hpp"
#include "internal/input/controller.hpp"
#include "internal/input/gamepad.hpp"
#include "internal/input/pimpl/controller.hpp"
#include "internal/input/pimpl/input_manager.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace argus::input {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Controller));

    double get_global_deadzone_radius(double radius);

    void set_global_deadzone_radius(double radius);

    DeadzoneShape get_global_deadzone_shape(void);

    void set_global_deadzone_shape(DeadzoneShape shape);

    Controller::Controller(const std::string &name):
        m_pimpl(&g_pimpl_pool.construct<pimpl_Controller>(name)) {
    }

    Controller::~Controller(void) {
        if (m_pimpl != nullptr) {
            g_pimpl_pool.destroy(m_pimpl);
        }
    }

    const std::string &Controller::get_name(void) const {
        return m_pimpl->name;
    }

    bool Controller::has_gamepad(void) const {
        return m_pimpl->attached_gamepad.has_value();
    }

    void Controller::attach_gamepad(HidDeviceId id) {
        if (m_pimpl->attached_gamepad.has_value()) {
            throw std::invalid_argument("Controller already has associated gamepad");
        }

        assoc_gamepad(id, this->get_name());
        m_pimpl->attached_gamepad = id;

        Logger::default_logger().info("Attached gamepad '%s' to controller '%s'",
                this->get_gamepad_name().c_str(), this->get_name().c_str());
    }

    bool Controller::attach_first_available_gamepad(void) {
        if (m_pimpl->attached_gamepad.has_value()) {
            throw std::invalid_argument("Controller already has associated gamepad");
        }

        auto id = assoc_first_available_gamepad(this->get_name());
        if (id < 0) {
            return false;
        }

        m_pimpl->attached_gamepad = id;

        Logger::default_logger().info("Attached gamepad '%s' to controller '%s'",
                this->get_gamepad_name().c_str(), this->get_name().c_str());

        return true;
    }

    void Controller::detach_gamepad(void) {
        if (!m_pimpl->attached_gamepad.has_value()) {
            // silently fail
            return;
        }

        unassoc_gamepad(m_pimpl->attached_gamepad.value());

        m_pimpl->attached_gamepad = std::nullopt;
    }

    std::string Controller::get_gamepad_name(void) {
        if (!this->has_gamepad()) {
            throw std::runtime_error("Controller does not have associated gamepad");
        }

        return ::argus::input::get_gamepad_name(m_pimpl->attached_gamepad.value());
    }

    double Controller::get_deadzone_radius(void) {
        return m_pimpl->dz_radius.value_or(InputManager::instance().get_global_deadzone_radius());
    }

    void Controller::set_deadzone_radius(double radius) {
        m_pimpl->dz_radius = std::min(std::max(radius, 0.0), 1.0);
    }

    void Controller::clear_deadzone_radius(void) {
        m_pimpl->dz_radius.reset();
    }

    DeadzoneShape Controller::get_deadzone_shape(void) {
        return m_pimpl->dz_shape.value_or(InputManager::instance().get_global_deadzone_shape());
    }

    void Controller::set_deadzone_shape(DeadzoneShape shape) {
        if (shape >= DeadzoneShape::MaxValue) {
            throw std::invalid_argument("Invalid deadzone shape ordinal "
                    + std::to_string(std::underlying_type_t<decltype(shape)>(shape)));
        }
        m_pimpl->dz_shape = shape;
    }

    void Controller::clear_deadzone_shape(void) {
        m_pimpl->dz_shape.reset();
    }

    static void _check_axis(GamepadAxis axis) {
        if (axis >= GamepadAxis::MaxValue) {
            throw std::invalid_argument("Invalid gamepad axis ordinal "
                    + std::to_string(std::underlying_type_t<decltype(axis)>(axis)));
        }
    }

    double Controller::get_axis_deadzone_radius(GamepadAxis axis) {
        _check_axis(axis);
        return m_pimpl->dz_axis_radii.at(size_t(axis)).value_or(
                m_pimpl->dz_radius.value_or(
                        InputManager::instance().get_global_axis_deadzone_radius(axis)));
    }

    void Controller::set_axis_deadzone_radius(GamepadAxis axis, double radius) {
        _check_axis(axis);
        m_pimpl->dz_axis_radii.at(size_t(axis)) = std::min(std::max(radius, 0.0), 1.0);
    }

    void Controller::clear_axis_deadzone_radius(GamepadAxis axis) {
        _check_axis(axis);
        m_pimpl->dz_axis_radii.at(size_t(axis)).reset();
    }

    DeadzoneShape Controller::get_axis_deadzone_shape(GamepadAxis axis) {
        _check_axis(axis);
        return m_pimpl->dz_axis_shapes.at(size_t(axis)).value_or(
                m_pimpl->dz_shape.value_or(
                        InputManager::instance().get_global_axis_deadzone_shape(axis)));
    }

    void Controller::set_axis_deadzone_shape(GamepadAxis axis, DeadzoneShape shape) {
        _check_axis(axis);
        m_pimpl->dz_axis_shapes.at(size_t(axis)) = shape;
    }

    void Controller::clear_axis_deadzone_shape(GamepadAxis axis) {
        _check_axis(axis);
        m_pimpl->dz_axis_shapes.at(size_t(axis)).reset();
    }

    void Controller::unbind_action(const std::string &action) {
        if (m_pimpl->action_to_key_bindings.find(action) == m_pimpl->action_to_key_bindings.end()) {
            return;
        }

        // remove action from binding list of keys it's bound to
        for (auto key : m_pimpl->action_to_key_bindings[action]) {
            remove_from_vector(m_pimpl->key_to_action_bindings[key], action);
        }

        // remove binding list of action
        m_pimpl->action_to_key_bindings.erase(action);
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
        auto it = m_pimpl->key_to_action_bindings.find(key);
        if (it != m_pimpl->key_to_action_bindings.end()) {
            return it->second; // implicitly deep-copied
        }

        // no bindings so just return empty vector
        return {};
    }

    std::vector<KeyboardScancode> Controller::get_keyboard_action_bindings(const std::string &action) const {
        auto it = m_pimpl->action_to_key_bindings.find(action);
        if (it != m_pimpl->action_to_key_bindings.end()) {
            return it->second; // implicitly deep-copied
        }

        // no bindings so just return empty vector
        return {};
    }

    void Controller::bind_keyboard_key(KeyboardScancode key, const std::string &action) {
        _bind_thing(m_pimpl->key_to_action_bindings, m_pimpl->action_to_key_bindings, key, action);
    }

    void Controller::unbind_keyboard_key(KeyboardScancode key) {
        _unbind_thing(m_pimpl->key_to_action_bindings, m_pimpl->action_to_key_bindings, key);
    }

    void Controller::unbind_keyboard_key(KeyboardScancode key, const std::string &action) {
        _unbind_thing(m_pimpl->key_to_action_bindings, m_pimpl->action_to_key_bindings, key, action);
    }

    void Controller::bind_mouse_button(MouseButton button, const std::string &action) {
        _bind_thing(m_pimpl->mouse_button_to_action_bindings, m_pimpl->action_to_mouse_button_bindings, button, action);
    }

    void Controller::unbind_mouse_button(MouseButton button) {
        _unbind_thing(m_pimpl->mouse_button_to_action_bindings, m_pimpl->action_to_mouse_button_bindings, button);
    }

    void Controller::unbind_mouse_button(MouseButton button, const std::string &action) {
        _unbind_thing(m_pimpl->mouse_button_to_action_bindings, m_pimpl->action_to_mouse_button_bindings, button,
                action);//TODO
    }

    void Controller::bind_mouse_axis(MouseAxis axis, const std::string &action) {
        _bind_thing(m_pimpl->mouse_axis_to_action_bindings, m_pimpl->action_to_mouse_axis_bindings, axis, action);
    }

    void Controller::unbind_mouse_axis(MouseAxis axis) {
        _unbind_thing(m_pimpl->mouse_axis_to_action_bindings, m_pimpl->action_to_mouse_axis_bindings, axis);
    }

    void Controller::unbind_mouse_axis(MouseAxis axis, const std::string &action) {
        _unbind_thing(m_pimpl->mouse_axis_to_action_bindings, m_pimpl->action_to_mouse_axis_bindings, axis, action);
    }

    void Controller::bind_gamepad_button(GamepadButton button, const std::string &action) {
        _bind_thing(m_pimpl->gamepad_button_to_action_bindings, m_pimpl->action_to_gamepad_button_bindings, button,
                action);
    }

    void Controller::unbind_gamepad_button(GamepadButton button) {
        _unbind_thing(m_pimpl->gamepad_button_to_action_bindings, m_pimpl->action_to_gamepad_button_bindings, button);
    }

    void Controller::unbind_gamepad_button(GamepadButton button, const std::string &action) {
        _unbind_thing(m_pimpl->gamepad_button_to_action_bindings, m_pimpl->action_to_gamepad_button_bindings,
                button, action);
    }

    void Controller::bind_gamepad_axis(GamepadAxis button, const std::string &action) {
        _bind_thing(m_pimpl->gamepad_axis_to_action_bindings, m_pimpl->action_to_gamepad_axis_bindings, button, action);
    }

    void Controller::unbind_gamepad_axis(GamepadAxis button) {
        _unbind_thing(m_pimpl->gamepad_axis_to_action_bindings, m_pimpl->action_to_gamepad_axis_bindings, button);
    }

    void Controller::unbind_gamepad_axis(GamepadAxis button, const std::string &action) {
        _unbind_thing(m_pimpl->gamepad_axis_to_action_bindings, m_pimpl->action_to_gamepad_axis_bindings,
                button, action);
    }

    bool Controller::is_gamepad_button_pressed(GamepadButton button) {
        if (!has_gamepad()) {
            throw std::runtime_error("Cannot query gamepad button state for controller: No gamepad is associated");
        }

        return ::argus::input::is_gamepad_button_pressed(m_pimpl->attached_gamepad.value(), button);
    }

    double Controller::get_gamepad_axis(GamepadAxis axis) {
        if (!has_gamepad()) {
            throw std::runtime_error("Cannot query gamepad axis state for controller: No gamepad is associated");
        }

        return ::argus::input::get_gamepad_axis(m_pimpl->attached_gamepad.value(), axis);
    }

    double Controller::get_gamepad_axis_delta(GamepadAxis axis) {
        if (!has_gamepad()) {
            throw std::runtime_error("Cannot query gamepad axis state for controller: No gamepad is associated");
        }

        return ::argus::input::get_gamepad_axis_delta(m_pimpl->attached_gamepad.value(), axis);
    }

    bool Controller::is_action_pressed(const std::string &action) {
        auto kb_it = m_pimpl->action_to_key_bindings.find(action);
        if (kb_it != m_pimpl->action_to_key_bindings.cend()) {
            for (auto key : kb_it->second) {
                if (is_key_pressed(key)) {
                    return true;
                }
            }
        }

        if (this->has_gamepad()) {
            auto gamepad_it = m_pimpl->action_to_gamepad_button_bindings.find(action);
            if (gamepad_it != m_pimpl->action_to_gamepad_button_bindings.cend()) {
                for (auto btn : gamepad_it->second) {
                    if (this->is_gamepad_button_pressed(btn)) {
                        return true;
                    }
                }
            }
        }

        auto mouse_it = m_pimpl->action_to_mouse_button_bindings.find(action);
        if (mouse_it != m_pimpl->action_to_mouse_button_bindings.cend()) {
            for (auto btn : mouse_it->second) {
                if (is_mouse_button_pressed(btn)) {
                    return true;
                }
            }
        }

        return false;
    }

    double Controller::get_action_axis(const std::string &action) {
        if (this->has_gamepad()) {
            auto gamepad_it = m_pimpl->action_to_gamepad_axis_bindings.find(action);
            if (gamepad_it != m_pimpl->action_to_gamepad_axis_bindings.cend() && !gamepad_it->second.empty()) {
                return this->get_gamepad_axis(gamepad_it->second.front());
            }
        }

        auto mouse_it = m_pimpl->action_to_mouse_axis_bindings.find(action);
        if (mouse_it != m_pimpl->action_to_mouse_axis_bindings.cend() && !mouse_it->second.empty()) {
            return get_mouse_axis(mouse_it->second.front());
        }

        return 0;
    }

    double Controller::get_action_axis_delta(const std::string &action) {
        if (this->has_gamepad()) {
            auto gamepad_it = m_pimpl->action_to_gamepad_axis_bindings.find(action);
            if (gamepad_it != m_pimpl->action_to_gamepad_axis_bindings.cend() && !gamepad_it->second.empty()) {
                return this->get_gamepad_axis_delta(gamepad_it->second.front());
            }
        }

        auto mouse_it = m_pimpl->action_to_mouse_axis_bindings.find(action);
        if (mouse_it != m_pimpl->action_to_mouse_axis_bindings.cend() && !mouse_it->second.empty()) {
            return get_mouse_axis_delta(mouse_it->second.front());
        }

        return 0;
    }

    void ack_gamepad_disconnects(void) {
        for (auto &[_, controller] : InputManager::instance().m_pimpl->controllers) {
            if (controller->m_pimpl->was_gamepad_disconnected) {
                // acknowledge disconnect flag set by render thread and fully
                // disassoc gamepad from controller
                controller->m_pimpl->was_gamepad_disconnected = false;
                controller->detach_gamepad();
            }
        }
    }
}

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
#include "internal/input/event_helpers.hpp"
#include "internal/input/input_manager.hpp"
#include "internal/input/pimpl/controller.hpp"
#include "internal/input/pimpl/input_manager.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string>
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
            for (const auto &[_, controller] : pimpl->controllers) {
                delete controller;
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

    Controller &InputManager::add_controller(const std::string &name) {
        if (pimpl->controllers.size() >= MAX_CONTROLLERS) {
            throw std::invalid_argument("Controller limit reached");
        }

        auto controller = new Controller(name);

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

    double InputManager::get_global_deadzone_radius(void) {
        return pimpl->dz_radius;
    }

    void InputManager::set_global_deadzone_radius(double radius) {
        pimpl->dz_radius = std::min(std::max(radius, 0.0), 1.0);
    }

    DeadzoneShape InputManager::get_global_deadzone_shape(void) {
        return pimpl->dz_shape;
    }

    void InputManager::set_global_deadzone_shape(DeadzoneShape shape) {
        if (shape >= DeadzoneShape::MaxValue) {
            throw std::invalid_argument("Invalid deadzone shape ordinal "
                    + std::to_string(std::underlying_type_t<decltype(shape)>(shape)));
        }
        pimpl->dz_shape = shape;
    }

    static void _check_axis(GamepadAxis axis) {
        if (axis >= GamepadAxis::MaxValue) {
            throw std::invalid_argument("Invalid gamepad axis ordinal "
                    + std::to_string(std::underlying_type_t<decltype(axis)>(axis)));
        }
    }

    double InputManager::get_global_axis_deadzone_radius(GamepadAxis axis) {
        _check_axis(axis);
        return pimpl->dz_axis_radii.at(size_t(axis)).value_or(pimpl->dz_radius);
    }

    void InputManager::set_global_axis_deadzone_radius(GamepadAxis axis, double radius) {
        _check_axis(axis);
        pimpl->dz_axis_radii.at(size_t(axis)) = std::min(std::max(radius, 0.0), 1.0);
    }

    void InputManager::clear_global_axis_deadzone_radius(GamepadAxis axis) {
        _check_axis(axis);
        pimpl->dz_axis_radii.at(size_t(axis)).reset();
    }

    DeadzoneShape InputManager::get_global_axis_deadzone_shape(GamepadAxis axis) {
        _check_axis(axis);
        return pimpl->dz_axis_shapes.at(size_t(axis)).value_or(pimpl->dz_shape);
    }

    void InputManager::set_global_axis_deadzone_shape(GamepadAxis axis, DeadzoneShape shape) {
        _check_axis(axis);
        pimpl->dz_axis_shapes.at(size_t(axis)) = shape;
    }

    void InputManager::clear_global_axis_deadzone_shape(GamepadAxis axis) {
        _check_axis(axis);
        pimpl->dz_axis_shapes.at(size_t(axis)).reset();
    }
}

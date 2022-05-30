/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/error_util.hpp"

#include "argus/ecs/system.hpp"
#include "argus/ecs/system_builder.hpp"
#include "internal/ecs/module_ecs.hpp"
#include "internal/ecs/system.hpp"
#include "internal/ecs/pimpl/system.hpp"

#include <stdexcept>
#include <vector>

namespace argus {
    std::vector<System*> g_systems;

    SystemBuilder System::builder(void) {
        validate_state(!g_ecs_initialized, "Systems may not be registered beyond the init lifecycle stage");

        return SystemBuilder();
    }

    System &System::create(std::string name, std::vector<std::type_index> component_types, EntityCallback callback) {
        validate_state(!g_ecs_initialized, "Systems may not be registered beyond the init lifecycle stage");
        validate_arg(!name.empty(), "System name must be non-empty");
        validate_arg(!component_types.empty(), "At least one component type must be supplied for system");
        validate_arg(callback != nullptr, "System callback must be non-null");

        auto system = new System(name, component_types, callback);
        g_systems.push_back(system);
        return *system;
    }

    System::System(std::string name, std::vector<std::type_index> component_types, EntityCallback callback):
        pimpl(new pimpl_System(name, component_types, callback, true)) {
    }

    System::~System(void) {
        delete pimpl;
    }

    const std::string System::get_name(void) {
        return pimpl->name;
    }

    bool System::is_active(void) {
        return pimpl->active;
    }

    void System::set_active(bool active) {
        pimpl->active = active;
    }

}

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

#pragma once

#include "argus/ecs/system.hpp"

#include <string>
#include <typeindex>
#include <vector>

namespace argus {
    struct pimpl_System {
        std::string name;
        std::vector<std::type_index> component_types;
        EntityCallback callback;
        bool active;

        pimpl_System(std::string name, std::vector<std::type_index> component_types, EntityCallback callback,
                bool active):
            name(name),
            component_types(component_types),
            callback(callback),
            active(active) {
        }
    };
}

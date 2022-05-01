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

#pragma once

#include "argus/lowlevel/memory.hpp"

#include "argus/ecs/component_type_registry.hpp"

#include <map>

namespace argus {
    struct ComponentTypeInfo {
        ComponentTypeId id;
        //std::string name;
        size_t size;

        // clang-format off
        ComponentTypeInfo(ComponentTypeId id, size_t size):
                id(id),
                //name(name),
                size(size) {
        }
        // clang-format on
    };

    struct pimpl_ComponentTypeRegistry {
        std::map<std::type_index, ComponentTypeInfo> component_types;
        ComponentTypeId next_id = 0;
        AllocPool *component_pools;
        bool sealed = false;
    };

}

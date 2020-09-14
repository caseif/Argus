/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module ecs
#include "argus/ecs/component_type_registry.hpp"

#include <map>
#include <vector>

namespace argus {

    struct ComponentTypeInfo {
        ComponentTypeId id;
        std::string name;
        size_t size;

        // clang-format off
        ComponentTypeInfo(ComponentTypeId id, std::string name, size_t size):
                id(id),
                name(name),
                size(size) {
        }
        // clang-format on
    };

    struct pimpl_ComponentTypeRegistry {
        std::vector<ComponentTypeInfo> component_types;
        ComponentTypeId next_id = 0;
        AllocPool **component_pools;
        bool sealed = false;
    };

}

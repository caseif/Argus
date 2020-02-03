/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <map>
#include <vector>

// module lowlevel
#include "argus/memory.hpp"

// module ecs
#include "argus/ecs/component_registry.hpp"

namespace argus {

    struct ComponentInfo {
        ComponentId id;
        std::string name;
        size_t size;

        ComponentInfo(ComponentId id, std::string name, size_t size):
                id(id),
                name(name),
                size(size) {
        }
    };
    
    struct pimpl_ComponentRegistry {
        std::vector<ComponentInfo> component_types;
        ComponentId next_id = 0;
        std::map<ComponentId, AllocPool> component_pools;
        bool sealed = false;
    };

}

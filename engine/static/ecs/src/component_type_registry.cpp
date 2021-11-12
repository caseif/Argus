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

// module lowlevel
#include "argus/lowlevel/memory.hpp"
#include "internal/lowlevel/error_util.hpp"

// module ecs
#include "argus/ecs/component_type_registry.hpp"
#include "internal/ecs/pimpl/component_type_registry.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <cctype>
#include <cstdlib>

namespace argus {

    static ComponentTypeRegistry *g_comp_reg_singleton;

    inline static std::string to_lower(std::string &str) {
        std::string str_copy = str;
        std::transform(str_copy.begin(), str_copy.end(), str_copy.begin(),
                [](auto c) { return std::tolower(c); });
        return str_copy;
    }

    inline static ComponentTypeInfo *_lookup_component_type(std::vector<ComponentTypeInfo> &component_types,
            std::function<bool(ComponentTypeInfo)> predicate) {
        auto res = std::find_if(component_types.begin(), component_types.end(), predicate);
        if (res == component_types.cend()) {
            return nullptr;
        }
        return &(*res);
    }

    inline static ComponentTypeInfo *_lookup_component_type(std::vector<ComponentTypeInfo> &component_types,
            ComponentTypeId type_id) {
        return _lookup_component_type(component_types, [type_id](auto cmpnt) {
            return cmpnt.id == type_id;
        });
    }

    inline static ComponentTypeInfo *_lookup_component_type(std::vector<ComponentTypeInfo> &component_types,
            std::string &component_name) {
        return _lookup_component_type(component_types, [component_name](auto cmpnt) {
            return cmpnt.name == component_name;
        });
    }

    ComponentTypeRegistry::ComponentTypeRegistry(void):
            pimpl(new pimpl_ComponentTypeRegistry()) {
        //TODO
    }

    ComponentTypeRegistry &ComponentTypeRegistry::instance(void) {
        if (g_comp_reg_singleton == nullptr) {
            g_comp_reg_singleton = new ComponentTypeRegistry();
        }
        return *g_comp_reg_singleton;
    }

    void *ComponentTypeRegistry::alloc_component(ComponentTypeId component_type) {
        validate_arg(component_type < ComponentTypeRegistry::instance().get_component_type_count(),
            "Invalid component type ID " + std::to_string(component_type));
        
        return (*pimpl->component_pools)[component_type].alloc();
    }

    void ComponentTypeRegistry::free_component(ComponentTypeId component_type, void *ptr) {
        validate_arg(component_type < ComponentTypeRegistry::instance().get_component_type_count(),
            "Invalid component type ID " + std::to_string(component_type));

        (*pimpl->component_pools)[component_type].free(ptr);
    }

    size_t ComponentTypeRegistry::get_component_type_count(void) {
        return pimpl->next_id;
    }

    ComponentTypeId ComponentTypeRegistry::get_component_type_id(std::string &type_name) {
        std::string name_lower = type_name;
        to_lower(type_name);
        ComponentTypeInfo *component = _lookup_component_type(pimpl->component_types,
            [name_lower](auto cmpnt) {
                return cmpnt.name == name_lower;
            }
        );

        validate_arg(component == nullptr, "No component type registered with name " + type_name);

        return component->id;
    }

    size_t ComponentTypeRegistry::get_component_type_size(ComponentTypeId type_id) {
        ComponentTypeInfo *component = _lookup_component_type(pimpl->component_types, type_id);
        validate_arg(component == nullptr, "No component type registered with ID " + std::to_string(type_id));
        return component->size;
    }

    ComponentTypeId ComponentTypeRegistry::register_component_type(std::string &name, size_t size) {
        validate_arg(_lookup_component_type(pimpl->component_types, name) != nullptr,
            "Component type with name " + name + " is already registered");
        validate_state(!_is_sealed(), "Failed to register component type because registry is already sealed");

        ComponentTypeId new_id = pimpl->next_id++;
        pimpl->component_types.insert(pimpl->component_types.begin(), ComponentTypeInfo(new_id, name, size));

        return new_id;
    }

    void ComponentTypeRegistry::_seal(void) {
        validate_state(!pimpl->sealed, "Cannot seal component registry because it is already sealed.");

        pimpl->sealed = true;

        *pimpl->component_pools = static_cast<AllocPool*>(malloc(sizeof(AllocPool) * pimpl->next_id));
        for (size_t i = 0; i < pimpl->next_id; i++) {
            AllocPool &target = ((*pimpl->component_pools)[i]);
            new (&target) AllocPool(pimpl->component_types.at(i).size);
        }
    }

    bool ComponentTypeRegistry::_is_sealed(void) {
        return pimpl->sealed;
    }

}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module ecs
#include "argus/ecs/component_type_registry.hpp"
#include "internal/ecs/pimpl/component_type_registry.hpp"

#include <algorithm>
#include <functional>

namespace argus {

    static ComponentTypeRegistry *g_comp_reg_singleton;

    static AllocPool **g_component_pools;

    inline static std::string to_lower(std::string &str) {
        std::string str_copy = str;
        std::transform(str_copy.begin(), str_copy.end(), str_copy.begin(),
                [](unsigned char c) { return std::tolower(c); });
        return str_copy;
    }

    inline static ComponentTypeInfo *_lookup_component_type(std::vector<ComponentTypeInfo> &component_types,
            std::function<bool(ComponentTypeInfo)> predicate) {
        auto res = std::find_if(component_types.begin(), component_types.end(), predicate);
        if (res == component_types.cend()) {
            return nullptr;
        }
        return res.base();
    }

    inline static ComponentTypeInfo *_lookup_component_type(std::vector<ComponentTypeInfo> &component_types,
            ComponentTypeId type_id) {
        return _lookup_component_type(component_types, [type_id](ComponentTypeInfo cmpnt) {
            return cmpnt.id == type_id;
        });
    }

    inline static ComponentTypeInfo *_lookup_component_type(std::vector<ComponentTypeInfo> &component_types,
            std::string &component_name) {
        return _lookup_component_type(component_types, [component_name](ComponentTypeInfo cmpnt) {
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

    size_t ComponentTypeRegistry::get_component_type_count(void) {
        return pimpl->next_id;
    }

    ComponentTypeId ComponentTypeRegistry::get_component_type_id(std::string &type_name) {
        std::string name_lower = type_name;
        to_lower(type_name);
        ComponentTypeInfo *component = _lookup_component_type(pimpl->component_types, [name_lower](ComponentTypeInfo cmpnt) {
            return cmpnt.name == name_lower;
        });
        if (component == nullptr) {
            throw std::invalid_argument("No component type registered with name " + type_name);
        }
        return component->id;
    }

    size_t ComponentTypeRegistry::get_component_type_size(ComponentTypeId type_id) {
        ComponentTypeInfo *component = _lookup_component_type(pimpl->component_types, type_id);
        if (component == nullptr) {
            throw std::invalid_argument("No component type registered with ID " + type_id);
        }
        return component->size;
    }

    ComponentTypeId ComponentTypeRegistry::register_component(std::string &name, size_t size) {
        if (_lookup_component_type(pimpl->component_types, name) != nullptr) {
            throw std::invalid_argument("Component type with name " + name + " is already registered");
        }

        _ARGUS_ASSERT(!_is_sealed(), "Failed to register component type because registry is already sealed");

        pimpl->component_types.insert(pimpl->component_types.begin(), ComponentTypeInfo(pimpl->next_id++, name, size));
    }

    void ComponentTypeRegistry::_seal(void) {
        _ARGUS_ASSERT(!pimpl->sealed, "Cannot seal component registry because it is already sealed.\n");
        pimpl->sealed = true;

        *g_component_pools = static_cast<AllocPool*>(malloc(sizeof(AllocPool) * pimpl->next_id));
        for (size_t i = 0; i < pimpl->next_id; i++) {
            AllocPool &target = ((*g_component_pools)[i]);
            //TODO: verify initial_cap is reasonable
            new (&target) AllocPool(pimpl->component_types.at(i).size, 32);
        }
    }

    bool ComponentTypeRegistry::_is_sealed(void) {
        return pimpl->sealed;
    }

}

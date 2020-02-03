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
#include "argus/ecs/component_registry.hpp"
#include "internal/ecs/pimpl/component_registry.hpp"

#include <algorithm>
#include <functional>

namespace argus {

    static ComponentRegistry *g_comp_reg_singleton;

    inline static std::string to_lower(std::string &str) {
        std::string str_copy = str;
        std::transform(str_copy.begin(), str_copy.end(), str_copy.begin(),
                [](unsigned char c) { return std::tolower(c); });
        return str_copy;
    }

    inline static ComponentInfo *_lookup_component(std::vector<ComponentInfo> &component_types,
            std::function<bool(ComponentInfo)> predicate) {
        auto res = std::find_if(component_types.begin(), component_types.end(), predicate);
        if (res == component_types.cend()) {
            return nullptr;
        }
        return res.base();
    }

    inline static ComponentInfo *_lookup_component(std::vector<ComponentInfo> &component_types,
            ComponentId component_id) {
        return _lookup_component(component_types, [component_id](ComponentInfo cmpnt) {
            return cmpnt.id == component_id;
        });
    }

    inline static ComponentInfo *_lookup_component(std::vector<ComponentInfo> &component_types,
            std::string &component_name) {
        return _lookup_component(component_types, [component_name](ComponentInfo cmpnt) {
            return cmpnt.name == component_name;
        });
    }

    ComponentRegistry::ComponentRegistry(void):
            pimpl(new pimpl_ComponentRegistry()) {
        //TODO
    }

    ComponentRegistry &ComponentRegistry::instance(void) {
        if (g_comp_reg_singleton == nullptr) {
            g_comp_reg_singleton = new ComponentRegistry();
        }
        return *g_comp_reg_singleton;
    }

    ComponentId ComponentRegistry::get_component_id(std::string &component_name) {
        std::string name_lower = component_name;
        to_lower(component_name);
        ComponentInfo *component = _lookup_component(pimpl->component_types, [name_lower](ComponentInfo cmpnt) {
            return cmpnt.name == name_lower;
        });
        if (component == nullptr) {
            throw std::invalid_argument("No component type registered with name " + component_name);
        }
        return component->id;
    }

    size_t ComponentRegistry::get_component_size(ComponentId component_id) {
        ComponentInfo *component = _lookup_component(pimpl->component_types, component_id);
        if (component == nullptr) {
            throw std::invalid_argument("No component type registered with ID " + component_id);
        }
        return component->size;
    }

    ComponentId ComponentRegistry::register_component(std::string &name, size_t size) {
        if (_lookup_component(pimpl->component_types, name) != nullptr) {
            throw std::invalid_argument("Component type with name " + name + " is already registered");
        }

        pimpl->component_types.insert(pimpl->component_types.begin(), ComponentInfo(pimpl->next_id++, name, size));
    }

    void ComponentRegistry::_seal(void) {
        _ARGUS_ASSERT(!pimpl->sealed, "Cannot seal component registry because it is already sealed.\n");
        pimpl->sealed = true;
    }

    bool ComponentRegistry::_is_sealed(void) {
        return pimpl->sealed;
    }

}

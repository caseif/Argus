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

#include "argus/lowlevel/error_util.hpp"
#include "argus/lowlevel/memory.hpp"

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

    ComponentTypeRegistry::ComponentTypeRegistry(void) :
            m_pimpl(new pimpl_ComponentTypeRegistry()) {
        //TODO
    }

    ComponentTypeRegistry &ComponentTypeRegistry::instance(void) {
        if (g_comp_reg_singleton == nullptr) {
            g_comp_reg_singleton = new ComponentTypeRegistry();
        }
        return *g_comp_reg_singleton;
    }

    void *ComponentTypeRegistry::alloc(std::type_index type) {
        auto info_it = m_pimpl->component_types.find(type);
        validate_arg(info_it != m_pimpl->component_types.end(), "Unregistered component type");

        return m_pimpl->component_pools[info_it->second.id].alloc();
    }

    void ComponentTypeRegistry::free(std::type_index type, void *ptr) {
        auto info_it = m_pimpl->component_types.find(type);
        validate_arg(info_it != m_pimpl->component_types.end(), "Unregistered component type");

        m_pimpl->component_pools[info_it->second.id].free(ptr);
    }

    void ComponentTypeRegistry::free(ComponentTypeId id, void *ptr) {
        validate_arg(id < get_type_count(), "Invalid component type ID " + std::to_string(id));

        m_pimpl->component_pools[id].free(ptr);
    }

    size_t ComponentTypeRegistry::get_type_count(void) {
        return m_pimpl->next_id;
    }

    ComponentTypeId ComponentTypeRegistry::get_id(std::type_index type) {
        auto info_it = m_pimpl->component_types.find(type);
        validate_arg(info_it != m_pimpl->component_types.end(), "Unregistered component type");

        return info_it->second.id;
    }

    void ComponentTypeRegistry::register_type(std::type_index type, size_t size) {
        validate_state(!_is_sealed(), "Failed to register component type because registry is already sealed");

        ComponentTypeId new_id = m_pimpl->next_id++;
        m_pimpl->component_types.insert({type, ComponentTypeInfo(new_id, size)});
    }

    void ComponentTypeRegistry::_seal(void) {
        validate_state(!m_pimpl->sealed, "Cannot seal component registry because it is already sealed.");

        m_pimpl->sealed = true;

        if (m_pimpl->next_id == 0) {
            // no need to allocate pools if no components are registered
            return;
        }

        m_pimpl->component_pools = static_cast<PoolAllocator *>(malloc(sizeof(PoolAllocator) * m_pimpl->next_id));
        for (auto [_, cmpt] : m_pimpl->component_types) {
            auto index = cmpt.id;
            PoolAllocator &target = m_pimpl->component_pools[index];
            new(&target) PoolAllocator(cmpt.size);
        }
    }

    bool ComponentTypeRegistry::_is_sealed(void) {
        return m_pimpl->sealed;
    }

}

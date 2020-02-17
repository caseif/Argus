/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/memory.hpp"
#include "internal/lowlevel/error_util.hpp"

// module ecs
#include "argus/ecs/component_type_registry.hpp"
#include "argus/ecs/entity.hpp"
#include "internal/ecs/pimpl/entity.hpp"

#include <initializer_list>
#include <stdexcept>

namespace argus {

    static AllocPool *g_entity_pool;

    Entity &Entity::create_entity(std::initializer_list<ComponentTypeId> component_types) {
        if (g_entity_pool == nullptr) {
            size_t entity_size = sizeof(pimpl_Entity)
                + (sizeof(void*) * ComponentTypeRegistry::instance().get_component_type_count());
            g_entity_pool = new AllocPool(entity_size, 64);
        }

        Entity &entity = *static_cast<Entity*>(g_entity_pool->alloc());

        for (auto cmp_type : component_types) {
            entity.pimpl->component_pointers[cmp_type] = ComponentTypeRegistry::instance().alloc_component(cmp_type);
        }

        return entity;
    }

    void Entity::destroy(void) {
        for (ComponentTypeId cmp_type = 0; cmp_type < ComponentTypeRegistry::instance().get_component_type_count();
                cmp_type++) {
            void *cmp_ptr = pimpl->component_pointers[cmp_type];
            if (cmp_ptr != nullptr) {
                ComponentTypeRegistry::instance().free_component(cmp_type, cmp_ptr);
            }
        }

        g_entity_pool->free(this);
    }

    EntityId Entity::get_id(void) {
        return pimpl->id;
    }

    void *Entity::get_component(ComponentTypeId component_type) {
        void *cmp_ptr = pimpl->component_pointers[component_type];
        validate_arg(cmp_ptr != nullptr, "Entity does not have component " + component_type);
        return cmp_ptr;
    }

    bool Entity::has_component(ComponentTypeId component_type) {
        if (component_type >= ComponentTypeRegistry::instance().get_component_type_count()) {
            throw std::invalid_argument("Invalid component ID\n");
        }
        return pimpl->component_pointers[component_type] != nullptr;
    }

}

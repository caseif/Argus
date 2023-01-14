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
#include "argus/lowlevel/vector.hpp"

#include "argus/ecs/component_type_registry.hpp"
#include "argus/ecs/entity.hpp"
#include "internal/ecs/entity.hpp"
#include "argus/ecs/entity_builder.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include <cstddef>

namespace argus {
    std::vector<const Entity*> g_created_entities;
    std::vector<EntityId> g_destroyed_entities;

    std::mutex g_entity_changes_mutex;

    //TODO: eventually implement some kind of periodic defragmentation routine that can run asynchronously
    static AllocPool *g_entity_pool;

    static size_t g_next_id;

    EntityBuilder Entity::builder(void) {
        return EntityBuilder();
    }

    Entity &Entity::create(std::vector<std::type_index> component_types) {
        if (g_entity_pool == nullptr) {
            size_t entity_size = sizeof(Entity)
                + (sizeof(void*) * ComponentTypeRegistry::instance().get_type_count());
            g_entity_pool = new AllocPool(entity_size);
        }

        Entity &entity = *static_cast<Entity*>(g_entity_pool->alloc());

        for (auto cmp_type : component_types) {
            auto cmp_id = ComponentTypeRegistry::instance().get_id(cmp_type);
            entity.component_pointers[cmp_id] = ComponentTypeRegistry::instance().alloc(cmp_type);
        }

        {
            g_entity_changes_mutex.lock();

            g_created_entities.push_back(&entity);

            g_entity_changes_mutex.unlock();
        }

        return entity;
    }

    Entity::Entity(void):
        id(g_next_id++) {
    }

    void Entity::destroy(void) {
        for (ComponentTypeId cmp_id = 0; cmp_id < ComponentTypeRegistry::instance().get_type_count(); cmp_id++) {
            void *cmp_ptr = component_pointers[cmp_id];
            if (cmp_ptr != nullptr) {
                ComponentTypeRegistry::instance().free(cmp_id, cmp_ptr);
            }
        }

        {
            g_entity_changes_mutex.lock();

            remove_from_vector(g_created_entities, this);
            g_destroyed_entities.push_back(this->get_id());

            g_entity_changes_mutex.unlock();
        }

        g_entity_pool->free(this);
    }

    EntityId Entity::get_id(void) const {
        return id;
    }

    void *Entity::get(std::type_index type) const {
        auto cmp_id = ComponentTypeRegistry::instance().get_id(type);
        void *cmp_ptr = component_pointers[cmp_id];
        validate_arg(cmp_ptr != nullptr, "Entity does not have component " "TODO");
        return cmp_ptr;
    }

    bool Entity::has(std::type_index type) const {
        auto cmp_id = ComponentTypeRegistry::instance().get_id(type);
        return component_pointers[cmp_id] != nullptr;
    }

}

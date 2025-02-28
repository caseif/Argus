/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/time.hpp"

#include "argus/ecs/entity.hpp"
#include "argus/ecs/system.hpp"
#include "internal/ecs/entity.hpp"
#include "internal/ecs/system.hpp"
#include "internal/ecs/system_executor.hpp"
#include "internal/ecs/pimpl/system.hpp"

#include <map>
#include <vector>

namespace argus {
    static std::map<const System *, std::map<EntityId, const Entity *>> g_entity_cache;

    static void execute_system(const System &system, TimeDelta delta, std::vector<const Entity *> &created_entities,
            std::vector<EntityId> &destroyed_entities) {
        auto &entities = g_entity_cache[&system];

        for (auto *entity : created_entities) {
            bool components_match = true;
            for (auto comp_type : system.m_pimpl->component_types) {
                if (!entity->has(comp_type)) {
                    components_match = false;
                    break;
                }
            }

            if (components_match) {
                entities[entity->get_id()] = entity;
            }
        }

        for (auto entity_id : destroyed_entities) {
            entities.erase(entity_id);
        }

        for (auto &[_, entity] : entities) {
            system.m_pimpl->callback(*entity, delta);
        }
    }

    void execute_all_systems(TimeDelta delta) {
        g_entity_changes_mutex.lock();

        // copy to local variables so we can quickly clear the authoritative versions
        auto created_entities = g_created_entities;
        auto destroyed_entities = g_destroyed_entities;

        g_created_entities.clear();
        g_destroyed_entities.clear();

        g_entity_changes_mutex.unlock();

        for (const auto &system : g_systems) {
            execute_system(*system, delta, created_entities, destroyed_entities);
        }
    }
}

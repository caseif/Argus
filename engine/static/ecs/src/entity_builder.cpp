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

#include "argus/ecs/component_type_registry.hpp"
#include "argus/ecs/entity.hpp"
#include "argus/ecs/entity_builder.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <typeindex>
#include <utility>

#include <cstring>

namespace argus {
    EntityBuilder::EntityBuilder(void) {
    }

    EntityBuilder &EntityBuilder::with(std::type_index type, std::function<void(void*)> deferred_init) {
        types[type] = deferred_init;
        return *this;
    }

    Entity &EntityBuilder::build(void) {
        std::vector<std::type_index> types_list;
        std::transform(types.begin(), types.end(), std::back_inserter(types_list),
            [](auto &pair){ return pair.first; });
        
        auto &entity = Entity::create(types_list);

        for (auto &pair : types) {
            if (pair.second != nullptr) {
                pair.second(entity.get(pair.first));
            }
        }

        return entity;
    }
}

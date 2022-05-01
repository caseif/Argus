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

#include <typeindex>
#include <vector>

namespace argus {
    EntityBuilder::EntityBuilder(void) {
    }

    EntityBuilder::~EntityBuilder(void) {
    }

    EntityBuilder &EntityBuilder::with(std::type_index type) {
        types.push_back(type);
        return *this;
    }

    Entity &EntityBuilder::build(void) {
        return Entity::create(this->types);
    }
}

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

#pragma once

#include "argus/ecs/component_type_registry.hpp"

#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

#include <cstdint>

namespace argus {
    typedef uint64_t EntityId;

    class EntityBuilder;

// disable non-standard extension warning for flexible array member
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4200)
#endif
    class Entity {
        private:
            const EntityId id;
            void *component_pointers[0];

            Entity(void);

            Entity(Entity&) = delete;
            Entity(Entity&&) = delete;

        public:
            static EntityBuilder builder(void);

            static Entity &create(std::vector<std::type_index> components);

            void destroy(void);

            EntityId get_id(void);

            void *get(std::type_index type);

            template <typename T>
            T &get() {
                return *static_cast<T*>(get(std::type_index(typeid(T))));
            }

            bool has(std::type_index type);

            template <typename T>
            bool has(void) {
                return has(std::type_index(typeid(T)));
            }
    };
#ifdef _MSC_VER
    #pragma warning(pop)
#endif
}

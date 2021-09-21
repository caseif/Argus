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

#include <initializer_list>
#include <utility>

#include <cstdint>

namespace argus {
    typedef uint64_t EntityId;

    struct pimpl_Entity;

    class Entity {
        private:
            pimpl_Entity *pimpl;

        public:
            static Entity &create_entity(std::initializer_list<ComponentTypeId> components);

            void destroy(void);

            EntityId get_id(void);

            void *get_component(ComponentTypeId component_type);

            template <typename T>
            T *get_component(ComponentTypeId component_type) {
                return static_cast<T*>(get_component(component_type));
            }

            bool has_component(ComponentTypeId component_type);
    };

}

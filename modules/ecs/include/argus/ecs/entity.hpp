/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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

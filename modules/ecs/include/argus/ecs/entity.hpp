/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
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
    typedef uint32_t EntityId;

    struct pimpl_Entity;

    class Entity {
        private:
            const pimpl_Entity *pimpl;

        public:
            static Entity &create_entity(std::initializer_list<std::pair<ComponentTypeId, void*>> components);

            void destroy(void);

            EntityId get_id(void);

            bool has_component(ComponentTypeId component_id);
    };

}

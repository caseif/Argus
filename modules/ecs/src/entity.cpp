/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module ecs
#include "argus/ecs/component_type_registry.hpp"
#include "argus/ecs/entity.hpp"
#include "internal/ecs/pimpl/entity.hpp"

#include <initializer_list>
#include <stdexcept>

namespace argus {

    Entity &Entity::create_entity(std::initializer_list<std::pair<ComponentTypeId, void*>> components) {
        //TODO
    }

    void Entity::destroy(void) {
        //TODO
    }

    EntityId Entity::get_id(void) {
        return pimpl->id;
    }

    bool Entity::has_component(ComponentTypeId component_id) {
        if (component_id >= ComponentTypeRegistry::instance().get_component_type_count()) {
            throw std::invalid_argument("Invalid component ID\n");
        }
        return pimpl->component_pointers[component_id] != nullptr;
    }

}

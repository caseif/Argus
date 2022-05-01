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

#pragma once

#include "argus/core/event.hpp"

#include "argus/resman/resource.hpp"

namespace argus {
    // forward declarations
    class Resource;

    /**
     * \brief A type of ResourceEvent.
     *
     * \sa ResourceEvent
     */
    enum class ResourceEventType {
        Load,
        Unload
    };

    /**
     * \brief An ArgusEvent pertaining to a Resource.
     *
     * \remark Resource events are dispatched after the Resource has been
     *         loaded or unloaded. Thus, when receiving an unload event,
     *         listeners should not expect the resource itself to be available.
     *
     * \sa ArgusEvent
     * \sa Resource
     */
    struct ResourceEvent : public ArgusEvent {
        /**
         * \brief The subtype of the event.
         */
        ResourceEventType subtype;

        /**
         * \brief The prototype of the resource associated with the event.
         */
        const ResourcePrototype prototype;

        /**
         * \brief The resource associated with the event.
         *
         * \warning This will be nullptr for resource unload events.
         */
        Resource *const resource;

        /**
         * \brief Constructs a new ResourceEvent.
         *
         * \param subtype The specific \link ResourceEventType type \endlink of
         *        ResourceEvent.
         * \param resource The Resource associated with the event.
         */
        ResourceEvent(const ResourceEventType subtype, ResourcePrototype prototype, Resource *resource):
                ArgusEvent { std::type_index(typeid(ResourceEvent)) },
                subtype(subtype),
                prototype(std::move(prototype)),
                resource(resource) {
        }
    };
}

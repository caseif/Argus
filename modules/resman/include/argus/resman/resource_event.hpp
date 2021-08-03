/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/core/event.hpp"

// module resman
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
                ArgusEvent { ArgusEventType::Resource },
                subtype(subtype),
                prototype(std::move(prototype)),
                resource(resource) {
        }
    };
}

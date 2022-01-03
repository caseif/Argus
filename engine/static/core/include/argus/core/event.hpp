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

// module core
#include "argus/core/callback.hpp"

#include <functional>
#include <typeindex>
#include <typeinfo>
#include <type_traits>

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace argus {
    /**
     * \brief Represents an event pertaining to the current application,
     *        typically triggered by user interaction.
     */
    struct ArgusEvent {
       protected:
        /**
         * \brief Aggregate constructor for ArgusEvent.
         *
         * \param type The type of event.
         */
        explicit ArgusEvent(std::type_index type);

       public:
        /**
         * \brief The type of event.
         */
        const std::type_index type;

        virtual ~ArgusEvent() {
        }
    };

    /**
     * \brief A callback that accepts an event and a piece of user-supplied
     *        data.
     */
    typedef std::function<void(const ArgusEvent &, void *)> ArgusEventCallback;

    enum class TargetThread {
        Update,
        Render
    };

    /**
     * \brief For internal use only. Please use
     *        \link argus::register_event_handler \endlink.
     *
     * \sa argus::register_event_handler
     */
    Index register_event_handler_with_type(std::type_index type, const ArgusEventCallback &callback,
            TargetThread target_thread, void *data);

    /**
     * \brief Registers a handler for particular events.
     *
     * Events which match the given filter will be passed to the callback
     * function along with the user-supplied data pointer.
     *
     * \tparam The type of event to handle.
     *
     * \param filter The \link ArgusEventFilter filter \endlink for the new
     *        event handler.
     * \param callback The \link ArgusEventCallback callback \endlink
     *        responsible for handling passed events.
     * \param target_thread The thread to invoke the handler function on.
     * \param data The data pointer to supply to the filter and callback
     *        functions on each invocation.
     *
     * \return The ID of the new registration.
     */
    template<typename EventType>
    Index register_event_handler(const std::function<void(const EventType&, void*)> &callback,
            const TargetThread target_thread, void *const data = nullptr) {
        return register_event_handler_with_type(std::type_index(typeid(EventType)),
                [callback](const ArgusEvent &e, void *d) {
                    assert(e.type == std::type_index(typeid(EventType)));
                    callback(reinterpret_cast<const EventType&>(e), d);
                },
                target_thread, data);
    }

    /**
     * \brief Unregisters an event handler.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_event_handler(Index id);

    /**
     * \brief Dispatches an event.
     *
     * This function is intended for internal use only, and is exposed here
     * solely due to C++ templating restrictions.
     *
     * \param event The event to be dispatched.
     */
    void _dispatch_event_ptr(ArgusEvent &event);

    /**
     * \brief Dispatches an event to all respective registered listeners.
     *
     * \param event An lreference to the event to be dispatched.
     */
    template <typename T, typename... Args>
    void dispatch_event(Args && ... args) {
        _dispatch_event_ptr(*new T(std::forward<Args>(args)...));
    }

    template<typename T>
    constexpr std::type_index type_index_of() {
        return std::type_index(typeid(T));
    }
}

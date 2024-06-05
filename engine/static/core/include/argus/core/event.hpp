/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/debug.hpp"

#include "argus/core/callback.hpp"
#include "argus/core/engine.hpp"
#include "argus/lowlevel/extra_type_traits.hpp"

#include <functional>
#include <type_traits>

#include <cstddef>
#include <cstdint>

namespace argus {
    template<typename, typename T>
    struct has_event_type_id_accessor : std::false_type {
    };

    template<typename T>
    struct has_event_type_id_accessor<T, std::void_t<decltype(T::get_event_type_id())>> : std::true_type {
    };

    template<typename T>
    constexpr bool has_event_type_id_accessor_v = has_event_type_id_accessor<T, void>::value;

    /**
     * @brief Represents an event pertaining to the current application,
     *        typically triggered by user interaction.
     */
    struct ArgusEvent {
      protected:
        /**
         * @brief Aggregate constructor for ArgusEvent.
         *
         * @param type_id The ID of the event type.
         */
        explicit ArgusEvent(std::string type_id);

      public:
        /**
         * @brief The ID of the event type.
         */
        const std::string type_id;

        ArgusEvent(const ArgusEvent &rhs);

        ArgusEvent(ArgusEvent &&rhs) = delete;

        ArgusEvent &operator=(const ArgusEvent &rhs) = delete;

        ArgusEvent &operator=(ArgusEvent &&rhs) = delete;

        virtual ~ArgusEvent() = default;
    };

    /**
     * @brief A callback that accepts an event and a piece of user-supplied
     *        data.
     */
    typedef std::function<void(const ArgusEvent &)> ArgusEventCallback;

    typedef std::function<void(const ArgusEvent &, void *)> ArgusEventWithDataCallback;

    typedef void(*ArgusEventHandlerUnregisterCallback)(Index, void *);

    enum class TargetThread {
        Update,
        Render
    };

    /**
     * @brief For internal use only. Please use
     *        \link argus::register_event_handler \endlink.
     *
     * @sa argus::register_event_handler
     */
    Index register_event_handler_with_type(std::string type_id, ArgusEventWithDataCallback callback,
            TargetThread target_thread, void *data, Ordering ordering,
            ArgusEventHandlerUnregisterCallback unregister_callback);

    /**
     * @brief Registers a handler for particular events.
     *
     * Events which match the given filter will be passed to the callback
     * function along with the user-supplied data pointer.
     *
     * @tparam EventType The type of event to handle.
     *
     * @param callback The \link ArgusEventCallback callback \endlink
     *        responsible for handling passed events.
     * @param target_thread The thread to invoke the handler function on.
     *
     * @return The ID of the new registration.
     */
    template<typename EventType>
    Index register_event_handler(std::function<void(const EventType &)> callback,
            const TargetThread target_thread, Ordering ordering = Ordering::Standard) {
        static_assert(has_event_type_id_accessor_v<EventType>,
                "Event class must contain static function get_event_type_id");
        return register_event_handler_with_type(EventType::get_event_type_id(),
                [callback = std::move(callback)](const ArgusEvent &e, void *d) {
                    UNUSED(d);
                    assert(e.type_id == EventType::get_event_type_id());
                    callback(reinterpret_cast<const EventType &>(e));
                },
                target_thread, nullptr, ordering, nullptr);
    }

    /**
     * @brief Registers a handler for particular events.
     *
     * Events which match the given filter will be passed to the callback
     * function along with the user-supplied data pointer.
     *
     * @tparam EventType The type of event to handle.
     *
     * @param callback The \link ArgusEventCallback callback \endlink
     *        responsible for handling passed events.
     * @param target_thread The thread to invoke the handler function on.
     *
     * @return The ID of the new registration.
     */
    template<typename EventType>
    Index register_event_handler(std::function<void(const EventType &, void *)> callback,
            const TargetThread target_thread, void *const data = nullptr,
            Ordering ordering = Ordering::Standard) {
        static_assert(has_event_type_id_accessor_v<EventType>,
                "Event class must contain static function get_event_type_id");
        return register_event_handler_with_type(EventType::get_event_type_id(),
                [callback = std::move(callback)](const ArgusEvent &e, void *d) {
                    assert(e.type_id == EventType::get_event_type_id());
                    callback(reinterpret_cast<const EventType &>(e), d);
                },
                target_thread, data, ordering, nullptr);
    }

    /**
     * @brief Unregisters an event handler.
     *
     * @param id The ID of the callback to unregister.
     */
    void unregister_event_handler(Index id);

    /**
     * @brief Dispatches an event.
     *
     * This function is intended for internal use only, and is exposed here
     * solely due to C++ templating restrictions.
     *
     * @param event The event to be dispatched.
     */
    void _dispatch_event_ptr(ArgusEvent &event);

    /**
     * @brief Dispatches an event to all respective registered listeners.
     *
     * @param args Arguments to be forwarded to the event constructor.
     */
    template<typename T, typename... Args>
    void dispatch_event(Args &&... args) {
        _dispatch_event_ptr(*new T(std::forward<Args>(args)...));
    }
}

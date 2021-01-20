/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/core/callback.hpp"

#include <functional>

#include <cstddef>
#include <cstdint>

namespace argus {

    /**
     * \brief Represents a class of event dispatched by the engine.
     */
    enum class ArgusEventType : uint16_t {
        /**
         * \brief An event of an unknown or undefined class.
         */
        UNDEFINED = 0x01,
        /**
         * \brief An event pertaining to a game window.
         */
        WINDOW = 0x02,
        /**
         * \brief An event pertaining to keyboard input.
         */
        KEYBOARD = 0x04,
        /**
         * \brief An event pertaining to mouse input.
         */
        MOUSE = 0x08,
        /**
         * \brief An event pertaining to joystick input.
         */
        JOYSTICK = 0x10,
        /**
         * \brief An event signifying some type of abstracted input.
         */
        INPUT = KEYBOARD | MOUSE | JOYSTICK
    };

    /**
     * \brief Bitwise OR implementation for ArgusEventType bitmask elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise OR of the operands.
     */
    constexpr ArgusEventType operator|(const ArgusEventType lhs, const ArgusEventType rhs);
    /**
     * \brief Bitwise OR-assignment implementation for ArgusEventType bitmask
     *        elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise OR of the operands.
     *
     * \sa ArgusEventType::operator|
     */
    constexpr inline ArgusEventType operator|=(const ArgusEventType lhs, const ArgusEventType rhs);
    /**
     * \brief Bitwise AND implementation for ArgusEventType bitmask elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise AND of the operands.
     */
    constexpr inline ArgusEventType operator&(const ArgusEventType lhs, const ArgusEventType rhs);

    /**
     * \brief Represents an event pertaining to the current application,
     *        typically triggered by user interaction.
     */
    struct ArgusEvent {
       protected:
        /**
         * \brief Aggregate constructor for ArgusEvent.
         *
         * \param type The \link ArgusEventType type \endlink of event.
         */
        ArgusEvent(ArgusEventType type);

       public:
        /**
         * \brief The type of event.
         */
        const ArgusEventType type;
    };

    /**
     * \brief A callback that accepts an event and a piece of user-supplied
     *        data.
     */
    typedef std::function<void(const ArgusEvent &, void *)> ArgusEventCallback;

    enum class TargetThread {
        UPDATE,
        RENDER
    };

    /**
     * \brief Registers a handler for particular events.
     *
     * Events which match the given filter will be passed to the callback
     * function along with the user-supplied data pointer.
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
    const Index register_event_handler(const ArgusEventType type, const ArgusEventCallback callback,
            const TargetThread target_thread, void *const data = nullptr);

    /**
     * \brief Unregisters an event handler.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_event_handler(const Index id);

    /**
     * \brief Dispatches an event.
     *
     * This function is intended for internal use only, and is exposed here
     * solely due to C++ templating restrictions.
     *
     * \param event The event to be dispatched.
     */
    void _dispatch_event_ptr(const ArgusEvent &event, size_t obj_size);

    /**
     * \brief Dispatches an event to all respective registered listeners.
     *
     * \param event An lreference to the event to be dispatched.
     */
    template <typename EventType>
    void dispatch_event(const EventType &event) {
        _dispatch_event_ptr(event, sizeof(event));
    }
}
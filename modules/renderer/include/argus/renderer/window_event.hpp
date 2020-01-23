/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

namespace argus {
    // forward declarations
    class Window;

    /**
     * \brief A type of WindowEvent.
     *
     * \sa WindowEvent
     */
    enum class WindowEventType {
        CLOSE,
        MINIMIZE,
        RESTORE
    };

    /**
     * \brief An ArgusEvent pertaining to a Window.
     *
     * \sa ArgusEvent
     * \sa Window
     */
    struct WindowEvent : public ArgusEvent {
        /**
         * \brief The specific \link WindowEventType type \endlink of
         *        WindowEvent.
         */
        const WindowEventType subtype;
        /**
         * \brief The Window associated with the event.
         */
        const Window &window;

        /**
         * \brief Constructs a new WindowEvent.
         *
         * \param subtype The specific \link WindowEventType type \endlink of
         *        WindowEvent.
         * \param window The Window associated with the event.
         */
        WindowEvent(const WindowEventType subtype, Window &window):
                ArgusEvent{ArgusEventType::WINDOW},
                subtype(subtype),
                window(window) {
        }
    };
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"

// module core
#include "argus/core/event.hpp"

namespace argus {
    // forward declarations
    class Window;

    /**
     * \brief A type of WindowEvent.
     *
     * \sa WindowEvent
     */
    enum class WindowEventType {
        Create,
        Update,
        RequestClose,
        Minimize,
        Restore,
        Focus,
        Unfocus,
        Resize,
        Move
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
        Window &window;

        /**
         * \brief The new resolution of the Window.
         *
         * \note This is populated only for resize events.
         */
        const Vector2u resolution;

        /**
         * \brief The new position of the Window.
         *
         * \note This is populated only for move events.
         */
        const Vector2i position;

        /**
         * \brief The \link TimeDelta delta \endlink of the current render
         *        frame.
         *
         * \note This is populated only for update events.
         */
        const TimeDelta delta;

        /**
         * \brief Constructs a new WindowEvent.
         *
         * \param subtype The specific \link WindowEventType type \endlink of
         *        WindowEvent.
         * \param window The Window associated with the event.
         */
        WindowEvent(WindowEventType subtype, Window &window):
                ArgusEvent{ArgusEventType::Window},
                subtype(subtype),
                window(window),
                resolution(),
                position(),
                delta() {
        }

        /**
         * \brief Constructs a new WindowEvent with the given data.
         *
         * \param subtype The specific \link WindowEventType type \endlink of
         *        WindowEvent.
         * \param window The Window associated with the event.
         * \param data The new position of resolution of the window following
         *        the event.
         */
        WindowEvent(WindowEventType subtype, Window &window, Vector2u resolution, Vector2i position,
                TimeDelta delta):
            ArgusEvent{ArgusEventType::Window},
            subtype(subtype),
            window(window),
            resolution(resolution),
            position(position),
            delta(delta) {
        }

        WindowEvent(WindowEvent &rhs):
            WindowEvent(rhs.subtype, rhs.window, rhs.resolution, rhs.position, rhs.delta) {
        }
    };
}

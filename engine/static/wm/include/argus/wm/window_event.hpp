/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/core/event.hpp"

#include <typeinfo>

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
        WindowEvent(WindowEventType subtype, Window &window) :
                ArgusEvent{std::type_index(typeid(WindowEvent))},
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
                TimeDelta delta) :
                ArgusEvent{std::type_index(typeid(WindowEvent))},
                subtype(subtype),
                window(window),
                resolution(resolution),
                position(position),
                delta(delta) {
        }

        WindowEvent(WindowEvent &rhs) :
                WindowEvent(rhs.subtype, rhs.window, rhs.resolution, rhs.position, rhs.delta) {
        }
    };
}

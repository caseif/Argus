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

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/pimpl/window.hpp"

namespace argus {
    WindowEvent::WindowEvent(WindowEventType subtype, Window &window, Vector2u resolution, Vector2i position,
            TimeDelta delta) :
            ArgusEvent{std::type_index(typeid(WindowEvent))},
            subtype(subtype),
            window(window),
            resolution(resolution),
            position(position),
            delta(delta) {
        window.pimpl->refcount.fetch_add(1);
    }

    WindowEvent::WindowEvent(WindowEventType subtype, Window &window) :
        WindowEvent(subtype, window, {}, {}, {}) {
    }

    WindowEvent::WindowEvent(const WindowEvent &rhs) :
        WindowEvent(rhs.subtype, rhs.window, rhs.resolution, rhs.position, rhs.delta) {
    }

    WindowEvent::~WindowEvent(void) {
        window.pimpl->refcount.fetch_sub(1);
    }
}

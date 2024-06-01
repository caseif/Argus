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

#include "argus/lowlevel/math/vector.hpp"
#include "argus/lowlevel/cabi/math.h"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "argus/wm/cabi/window.h"
#include "argus/wm/cabi/window_event.h"

using argus::Window;
using argus::WindowEvent;

static const WindowEvent &_as_ref(argus_window_event_const_t ptr) {
    return *reinterpret_cast<const WindowEvent *>(ptr);
}

#ifdef __cplusplus
extern "C" {
#endif

WindowEventType argus_window_event_get_subtype(argus_window_event_const_t self) {
    return WindowEventType(_as_ref(self).subtype);
}

argus_window_t argus_window_event_get_window(argus_window_event_const_t self) {
    return &_as_ref(self).window;
}

argus_vector_2u_t argus_window_event_get_resolution(argus_window_event_const_t self) {
    return as_c_vec(_as_ref(self).resolution);
}

argus_vector_2i_t argus_window_event_get_position(argus_window_event_const_t self) {
    return as_c_vec(_as_ref(self).position);
}

uint64_t argus_window_event_get_delta_us(argus_window_event_const_t self) {
    return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(_as_ref(self).delta).count());
}

#ifdef __cplusplus
}
#endif

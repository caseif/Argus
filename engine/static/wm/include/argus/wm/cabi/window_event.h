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

#ifdef __cplusplus
extern "C" {
#endif

#include "argus/lowlevel/cabi/math.h"

#include "argus/wm/cabi/window.h"

const char *EVENT_TYPE_WINDOW = "window";

enum WindowEventType {
    WINDOW_EVENT_TYPE_CREATE,
    WINDOW_EVENT_TYPE_UPDATE,
    WINDOW_EVENT_TYPE_REQUEST_CLOSE,
    WINDOW_EVENT_TYPE_MINIMIZE,
    WINDOW_EVENT_TYPE_RESTORE,
    WINDOW_EVENT_TYPE_FOCUS,
    WINDOW_EVENT_TYPE_UNFOCUS,
    WINDOW_EVENT_TYPE_RESIZE,
    WINDOW_EVENT_TYPE_MOVE,
};

typedef void *argus_window_event_t;
typedef const void *argus_window_event_const_t;

WindowEventType argus_window_event_get_subtype(argus_window_event_const_t self);

argus_window_t argus_window_event_get_window(argus_window_event_const_t self);

argus_vector_2u_t argus_window_event_get_resolution(argus_window_event_const_t self);

argus_vector_2i_t argus_window_event_get_position(argus_window_event_const_t self);

uint64_t argus_window_event_get_delta_us(argus_window_event_const_t self);

#ifdef __cplusplus
}
#endif

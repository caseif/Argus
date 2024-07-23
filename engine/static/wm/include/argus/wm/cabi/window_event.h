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

#ifdef __cplusplus
extern "C" {
#endif

#include "argus/lowlevel/cabi/math.h"

#include "argus/wm/cabi/window.h"

static const char *const k_event_type_window = "window";

typedef enum ArgusWindowEventType {
    ARGUS_WINDOW_EVENT_TYPE_CREATE,
    ARGUS_WINDOW_EVENT_TYPE_UPDATE,
    ARGUS_WINDOW_EVENT_TYPE_REQUEST_CLOSE,
    ARGUS_WINDOW_EVENT_TYPE_MINIMIZE,
    ARGUS_WINDOW_EVENT_TYPE_RESTORE,
    ARGUS_WINDOW_EVENT_TYPE_FOCUS,
    ARGUS_WINDOW_EVENT_TYPE_UNFOCUS,
    ARGUS_WINDOW_EVENT_TYPE_RESIZE,
    ARGUS_WINDOW_EVENT_TYPE_MOVE,
} ArgusWindowEventType;

typedef void *argus_window_event_t;
typedef const void *argus_window_event_const_t;

ArgusWindowEventType argus_window_event_get_subtype(argus_window_event_const_t self);

argus_window_t argus_window_event_get_window(argus_window_event_const_t self);

argus_vector_2u_t argus_window_event_get_resolution(argus_window_event_const_t self);

argus_vector_2i_t argus_window_event_get_position(argus_window_event_const_t self);

uint64_t argus_window_event_get_delta_us(argus_window_event_const_t self);

#ifdef __cplusplus
}
#endif

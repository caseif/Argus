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

#ifdef __cplusplus
extern "C" {
#endif

#include "argus/core/cabi/callback.h"
#include "argus/core/cabi/engine.h"

typedef void *argus_event_t;
typedef const void *argus_event_const_t;

typedef void (*argus_event_handler_t)(argus_event_const_t, void *);

typedef enum TargetThread {
    TARGET_THREAD_UPDATE,
    TARGET_THREAD_RENDER,
} TargetThread;

const char *argus_event_get_type_id(argus_event_const_t event);

Index argus_register_event_handler(const char *type_id, argus_event_handler_t handler,
        TargetThread target_thread, void *data, Ordering ordering);

void argus_unregister_event_handler(Index index);

void argus_dispatch_event(argus_event_t event);

#ifdef __cplusplus
}
#endif

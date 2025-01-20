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

#include "callback.h"
#include "module.h"

#include <stdbool.h>
#include <stdint.h>

#undef ARGUS_NORETURN
#ifdef __cplusplus
#define ARGUS_NORETURN [[noreturn]]
#else
#define ARGUS_NORETURN _Noreturn
#endif

typedef void(*nullary_callback_t)(void);
typedef void(*delta_callback_t)(uint64_t);

typedef enum Ordering {
    ORDERING_FIRST,
    ORDERING_EARLY,
    ORDERING_STANDARD,
    ORDERING_LATE,
    ORDERING_LAST
} Ordering;

void argus_initialize_engine(void);

ARGUS_NORETURN void argus_start_engine(delta_callback_t callback);

void argus_stop_engine(void);

ARGUS_NORETURN void argus_crash(const char *msg);

LifecycleStage argus_get_current_lifecycle_stage(void);

Index argus_register_update_callback(delta_callback_t update_callback, Ordering ordering);

void argus_unregister_update_callback(Index id);

Index argus_register_render_callback(delta_callback_t render_callback, Ordering ordering);

void argus_unregister_render_callback(Index id);

void argus_run_on_game_thread(nullary_callback_t callback);

bool argus_is_current_thread_update_thread(void);

#ifdef __cplusplus
}
#endif

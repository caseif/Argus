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

#include <stdbool.h>

typedef enum LifecycleStage {
    LIFECYCLE_STAGE_LOAD,
    LIFECYCLE_STAGE_PREINIT,
    LIFECYCLE_STAGE_INIT,
    LIFECYCLE_STAGE_POSTINIT,
    LIFECYCLE_STAGE_RUNNING,
    LIFECYCLE_STAGE_PREDEINIT,
    LIFECYCLE_STAGE_DEINIT,
    LIFECYCLE_STAGE_POSTDEINIT
} LifecycleStage;

typedef void(*lifecycle_update_callback_t)(LifecycleStage);

const char *argus_lifecycle_stage_to_str(LifecycleStage stage);

//TODO: argus_register_dynamic_module

bool argus_enable_dynamic_module(const char *module_id);

//TODO: argus_get_present_dynamic_modules

#ifdef __cplusplus
}
#endif

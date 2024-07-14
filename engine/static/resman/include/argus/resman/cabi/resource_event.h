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

#include "argus/resman/cabi/resource.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *k_event_type_resource = "resource";

typedef enum ResourceEventType {
    RESOURCE_EVENT_TYPE_LOAD,
    RESOURCE_EVENT_TYPE_UNLOAD,
} ResourceEventType;

typedef void *argus_resource_event_t;
typedef const void *argus_resource_event_const_t;

ResourceEventType argus_resource_event_get_subtype(argus_resource_event_const_t event);

const argus_resource_prototype_t argus_resource_event_get_prototype(argus_resource_event_const_t event);

argus_resource_t argus_resource_event_get_resource(argus_resource_event_t event);

#ifdef __cplusplus
}
#endif

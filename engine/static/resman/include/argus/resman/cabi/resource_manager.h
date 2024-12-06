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

#include "argus/resman/cabi/resource.h"

#include <stdbool.h>
#include <stddef.h>

typedef void *argus_resource_manager_t;
typedef const void *argus_resource_manager_const_t;

typedef void *argus_resource_loader_t;

typedef struct ResourceOrResourceError {
    bool is_ok;
    union {
        argus_resource_t value;
        argus_resource_error_t error;
    } ve;
} ResourceOrResourceError;

argus_resource_manager_t argus_resource_manager_get_instance(void);

void argus_resource_manager_discover_resources(argus_resource_manager_t mgr);

void argus_resource_manager_add_memory_package(argus_resource_manager_t mgr, const unsigned char *buf, size_t len);

void argus_resource_manager_register_loader(argus_resource_manager_t mgr, argus_resource_loader_t loader);

//TODO: register_extension_mappings

ResourceOrResourceError argus_resource_manager_get_resource(argus_resource_manager_t mgr, const char *uid);

ResourceOrResourceError argus_resource_manager_get_resource_weak(argus_resource_manager_t mgr, const char *uid);

ResourceOrResourceError argus_resource_manager_try_get_resource(argus_resource_manager_t mgr, const char *uid);

void argus_resource_manager_get_resource_async(argus_resource_manager_t mgr, const char *uid,
        void(*callback)(ResourceOrResourceError));

ResourceOrResourceError argus_resource_manager_create_resource(argus_resource_manager_t mgr, const char *uid,
        const char *media_type, const void *handle);

#ifdef __cplusplus
}
#endif

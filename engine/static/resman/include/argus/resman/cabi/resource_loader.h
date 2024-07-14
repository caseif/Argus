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

#include "argus/resman/cabi/resource_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *argus_resource_loader_t;
typedef const void *argus_resource_loader_const_t;

typedef struct VoidPtrOrResourceError {
    bool is_ok;
    union {
        void *value;
        argus_resource_error_t error;
    } ve;
} VoidPtrOrResourceError;

typedef void *argus_loaded_dependency_set_t;

typedef struct LoadedDependencySetOrResourceError {
    bool is_ok;
    union {
        argus_loaded_dependency_set_t value;
        argus_resource_error_t error;
    } ve;
} LoadedDependencySetOrResourceError;

typedef bool (*argus_resource_read_callback_t)(void *dst, size_t len,
        void *data);

typedef VoidPtrOrResourceError (*argus_resource_load_fn_t)(argus_resource_loader_t loader,
        argus_resource_manager_t manager, argus_resource_prototype_t proto,
        argus_resource_read_callback_t read_callback, size_t size, void *user_data, void *engine_data);

typedef VoidPtrOrResourceError (*argus_resource_copy_fn_t)(argus_resource_loader_t loader,
        argus_resource_manager_t manager, argus_resource_prototype_t proto,
        void *src, void *data);

typedef void (*argus_resource_unload_fn_t)(argus_resource_loader_t loader, void *ptr, void *user_data);

argus_resource_loader_t argus_resource_loader_new(
        const char *const *media_types,
        size_t media_types_count,
        argus_resource_load_fn_t load_fn,
        argus_resource_copy_fn_t copy_fn,
        argus_resource_unload_fn_t unload_fn,
        void *user_data
);

LoadedDependencySetOrResourceError argus_resource_loader_load_dependencies(argus_resource_loader_t loader,
        argus_resource_manager_t manager, const char *const *dependencies, size_t dependencies_count);

size_t argus_loaded_dependency_set_get_count(argus_loaded_dependency_set_t set);

const char *argus_loaded_dependency_set_get_name_at(argus_loaded_dependency_set_t set, size_t index);

argus_resource_const_t argus_loaded_dependency_set_get_resource_at(argus_loaded_dependency_set_t set, size_t index);

void argus_loaded_dependency_set_destruct(argus_loaded_dependency_set_t set);

#ifdef __cplusplus
}
#endif

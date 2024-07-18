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

#include <stdbool.h>
#include <stdint.h>

typedef struct ArgusHandle {
    uint32_t index;
    uint32_t uid;
} ArgusHandle;

typedef void *argus_handle_table_t;
typedef const void *argus_handle_table_const_t;

argus_handle_table_t argus_handle_table_new(void);

void argus_handle_table_delete(argus_handle_table_t table);

ArgusHandle argus_handle_table_create_handle(argus_handle_table_t table, void *ptr);

ArgusHandle argus_handle_table_copy_handle(argus_handle_table_t table, ArgusHandle handle);

bool argus_handle_table_update_handle(argus_handle_table_t table, ArgusHandle handle, void *ptr);

void argus_handle_table_release_handle(argus_handle_table_t table, ArgusHandle handle);

void *argus_handle_table_deref(argus_handle_table_const_t table, ArgusHandle handle);

#ifdef __cplusplus
}
#endif

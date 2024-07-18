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

#include "argus/lowlevel/cabi/handle.h"
#include "internal/lowlevel/cabi/handle.hpp"

#include "argus/lowlevel/handle.hpp"

static argus::HandleTable &_deref(argus_handle_table_t table) {
    return *reinterpret_cast<argus::HandleTable *>(table);
}

static const argus::HandleTable &_deref(argus_handle_table_const_t table) {
    return *reinterpret_cast<const argus::HandleTable *>(table);
}

argus_handle_table_t argus_handle_table_new(void) {
    return new argus::HandleTable();
}

void argus_handle_table_delete(argus_handle_table_t table) {
    delete reinterpret_cast<argus::HandleTable *>(table);
}

ArgusHandle argus_handle_table_create_handle(argus_handle_table_t table, void *ptr) {
    return wrap_handle(_deref(table).create_handle(ptr));
}

ArgusHandle argus_handle_table_copy_handle(argus_handle_table_t table, ArgusHandle handle) {
    return wrap_handle(_deref(table).copy_handle(unwrap_handle(handle)));
}

bool argus_handle_table_update_handle(argus_handle_table_t table, ArgusHandle handle, void *ptr) {
    return _deref(table).update_handle(unwrap_handle(handle), ptr);
}

void argus_handle_table_release_handle(argus_handle_table_t table, ArgusHandle handle) {
    _deref(table).release_handle(unwrap_handle(handle));
}

void *argus_handle_table_deref(argus_handle_table_const_t table, ArgusHandle handle) {
    return _deref(table).deref(unwrap_handle(handle));
}

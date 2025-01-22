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

#include "argus/scripting/cabi/error.h"
#include "argus/scripting/cabi/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void *argus_bound_type_def_t;
typedef const void *argus_bound_type_def_const_t;

typedef void *argus_bound_enum_def_t;
typedef const void *argus_bound_enum_def_const_t;

typedef void *argus_bound_function_def_t;
typedef const void *argus_bound_function_def_const_t;

typedef argus_object_wrapper_t (*ArgusFieldAccessor)(argus_object_wrapper_const_t inst, argus_object_type_const_t,
        const void *state);
typedef void (*ArgusFieldMutator)(argus_object_wrapper_t inst, argus_object_wrapper_t, const void *state);

argus_bound_type_def_t argus_create_type_def(const char *name, size_t size, const char *type_id, bool is_refable,
        ArgusCopyCtorProxy copy_ctor, ArgusMoveCtorProxy move_ctor, ArgusDtorProxy dtor);

argus_bound_enum_def_t argus_create_enum_def(const char *name, size_t width, const char *type_id);

ArgusMaybeBindingError argus_add_enum_value(argus_bound_enum_def_t def, const char *name, int64_t value);

ArgusMaybeBindingError argus_add_member_field(argus_bound_type_def_t, const char *name,
        argus_object_type_const_t field_type, ArgusFieldAccessor accessor, const void *accessor_state,
        ArgusFieldMutator mutator, const void *mutator_state);

ArgusMaybeBindingError argus_add_member_static_function(argus_bound_type_def_t def,
        const char *name, size_t params_count, const argus_object_type_const_t *params,
        argus_object_type_const_t ret_type, ArgusProxiedNativeFunction proxied_fn, void *extra);

ArgusMaybeBindingError argus_add_member_instance_function(argus_bound_type_def_t def,
        const char *name, bool is_const, size_t params_count, const argus_object_type_const_t *params,
        argus_object_type_const_t ret_type, ArgusProxiedNativeFunction proxied_fn, void *extra);

argus_bound_function_def_t argus_create_global_function_def(const char *name, bool is_const, size_t params_count,
        const argus_object_type_const_t *params, argus_object_type_const_t ret_type,
        ArgusProxiedNativeFunction proxied_fn, void *extra);

void argus_bound_type_def_delete(argus_bound_type_def_t def);

void argus_bound_enum_def_delete(argus_bound_enum_def_t def);

void argus_bound_function_def_delete(argus_bound_function_def_t def);

#ifdef __cplusplus
}

#endif

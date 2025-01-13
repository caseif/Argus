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

#include <stdbool.h>
#include <stddef.h>

typedef void(*ArgusCopyCtorProxy)(void *, const void *);

typedef void(*ArgusMoveCtorProxy)(void *, void *);

typedef void(*ArgusDtorProxy)(void *);

typedef enum ArgusIntegralType {
    INTEGRAL_TYPE_VOID,
    INTEGRAL_TYPE_INTEGER,
    INTEGRAL_TYPE_UINTEGER,
    INTEGRAL_TYPE_FLOAT,
    INTEGRAL_TYPE_BOOLEAN,
    INTEGRAL_TYPE_STRING,
    INTEGRAL_TYPE_STRUCT,
    INTEGRAL_TYPE_POINTER,
    INTEGRAL_TYPE_ENUM,
    INTEGRAL_TYPE_CALLBACK,
    INTEGRAL_TYPE_TYPE,
    INTEGRAL_TYPE_VECTOR,
    INTEGRAL_TYPE_VECTORREF,
    INTEGRAL_TYPE_RESULT,
} ArgusIntegralType;

typedef enum ArgusFunctionType {
    FUNCTION_TYPE_GLOBAL,
    FUNCTION_TYPE_MEMBER_STATIC,
    FUNCTION_TYPE_MEMBER_INSTANCE,
    FUNCTION_TYPE_EXTENSION,
} ArgusFunctionType;

typedef enum ArgusSymbolType {
    SYMBOL_TYPE_TYPE,
    SYMBOL_TYPE_FIELD,
    SYMBOL_TYPE_FUNCTION,
} ArgusSymbolType;

typedef void *argus_object_type_t;
typedef const void *argus_object_type_const_t;

typedef void *argus_script_callback_type_t;
typedef const void *argus_script_callback_type_const_t;

typedef void *argus_object_wrapper_t;
typedef const void *argus_object_wrapper_const_t;

struct ArgusScriptCallbackType;

typedef struct ArgusObjectWrapperOrReflectiveArgsError {
    bool is_err;
    argus_object_wrapper_t val;
    argus_reflective_args_error_t err;
} ArgusObjectWrapperOrReflectiveArgsError;

typedef struct ArgusObjectWrapperOrScriptInvocationError {
    bool is_err;
    argus_object_wrapper_t val;
    argus_script_invocation_error_t err;
} ArgusObjectWrapperOrScriptInvocationError;

typedef void *argus_script_callback_result_t;
typedef const void *argus_script_callback_result_const_t;

typedef ArgusObjectWrapperOrReflectiveArgsError(*ArgusProxiedNativeFunction)
        (size_t params_count, const argus_object_wrapper_t *params, const void *extra);

typedef void(*ArgusBareProxiedScriptCallback)(
        size_t params_count,
        argus_object_wrapper_t *params,
        const void *data,
        argus_script_callback_result_t out_result
);

typedef struct ArgusProxiedScriptCallback {
    ArgusBareProxiedScriptCallback bare_fn;
    const void *data;
} ArgusProxiedScriptCallback;

void argus_object_wrapper_or_refl_args_err_delete(ArgusObjectWrapperOrReflectiveArgsError res);

argus_object_type_t argus_object_type_new(ArgusIntegralType type, size_t size, bool is_const,
        bool is_refable, const char *type_id,
        argus_script_callback_type_const_t script_callback_type, argus_object_type_const_t primary_type,
        argus_object_type_const_t secondary_type);

void argus_object_type_delete(argus_object_type_t obj_type);

ArgusIntegralType argus_object_type_get_type(argus_object_type_const_t obj_type);

size_t argus_object_type_get_size(argus_object_type_const_t obj_type);

bool argus_object_type_get_is_const(argus_object_type_const_t obj_type);

bool argus_object_type_get_is_refable(argus_object_type_const_t obj_type);

const char *argus_object_type_get_type_id(argus_object_type_const_t obj_type);

const char *argus_object_type_get_type_name(argus_object_type_const_t obj_type);

argus_script_callback_type_const_t argus_object_type_get_callback_type(argus_object_type_const_t obj_type);

argus_object_type_const_t argus_object_type_get_primary_type(argus_object_type_const_t obj_type);

argus_object_type_const_t argus_object_type_get_secondary_type(argus_object_type_const_t obj_type);

argus_script_callback_type_t argus_script_callback_type_new(size_t param_count,
        const argus_object_type_const_t *param_types, argus_object_type_const_t return_type);

void argus_script_callback_type_delete(argus_script_callback_type_t callback_type);

size_t argus_script_callback_type_get_param_count(argus_script_callback_type_const_t callback_type);

void argus_script_callback_type_get_params(argus_script_callback_type_t callback_type,
        argus_object_type_t *obj_types, size_t count);

argus_object_type_t argus_script_callback_type_get_return_type(argus_script_callback_type_t callback_type);

argus_script_callback_result_t argus_script_callback_result_new(void);

void argus_script_callback_result_delete(argus_script_callback_result_t result);

void argus_script_callback_result_emplace(
        argus_script_callback_result_t dest,
        argus_object_wrapper_t value,
        argus_script_invocation_error_t error
);

bool argus_script_callback_result_is_ok(argus_script_callback_result_t result);

argus_object_wrapper_t argus_script_callback_result_get_value(argus_script_callback_result_t result);

argus_script_invocation_error_const_t argus_script_callback_result_get_error(argus_script_callback_result_t result);

argus_object_wrapper_t argus_object_wrapper_new(argus_object_type_const_t obj_type, size_t size);

void argus_object_wrapper_delete(argus_object_wrapper_t obj_wrapper);

argus_object_type_const_t argus_object_wrapper_get_type(argus_object_wrapper_const_t obj_wrapper);

const void *argus_object_wrapper_get_value(argus_object_wrapper_const_t obj_wrapper);

void *argus_object_wrapper_get_value_mut(argus_object_wrapper_t obj_wrapper);

bool argus_object_wrapper_is_on_heap(argus_object_wrapper_const_t obj_wrapper);

size_t argus_object_wrapper_get_buffer_size(argus_object_wrapper_const_t obj_wrapper);

bool argus_object_wrapper_is_initialized(argus_object_wrapper_const_t obj_wrapper);

#ifdef __cplusplus
}
#endif

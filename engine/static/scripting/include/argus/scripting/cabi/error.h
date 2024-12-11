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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *argus_binding_error_t;
typedef const void *argus_binding_error_const_t;

typedef enum {
    BINDING_ERROR_TYPE_DUPLICATE_NAME,
    BINDING_ERROR_TYPE_CONFLICTING_NAME,
    BINDING_ERROR_TYPE_INVALID_DEFINITION,
    BINDING_ERROR_TYPE_INVALID_MEMBERS,
    BINDING_ERROR_TYPE_UNKNOWN_PARENT,
    BINDING_ERROR_TYPE_OTHER,
} ArgusBindingErrorType;

void argus_binding_error_free(argus_binding_error_t err);

ArgusBindingErrorType argus_binding_error_get_type(argus_binding_error_const_t err);

const char *argus_binding_error_get_bound_name(argus_binding_error_const_t err);

const char *argus_binding_error_get_msg(argus_binding_error_const_t err);

typedef void *argus_reflective_args_error_t;
typedef const void *argus_reflective_args_error_const_t;

argus_reflective_args_error_t argus_reflective_args_error_new(const char *reason);

void argus_reflective_args_error_free(argus_reflective_args_error_t err);

const char *argus_reflective_args_error_get_reason(argus_reflective_args_error_const_t err);

#ifdef __cplusplus
}
#endif

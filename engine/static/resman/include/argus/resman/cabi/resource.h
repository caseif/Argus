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

typedef void *argus_resource_t;
typedef const void *argus_resource_const_t;

typedef struct argus_resource_prototype_t {
    const char *uid;
    const char *media_type;
    const char *fs_path;
} argus_resource_prototype_t;

typedef enum ResourceErrorReason {
    RESOURCE_ERROR_REASON_GENERIC,
    RESOURCE_ERROR_REASON_NOT_FOUND,
    RESOURCE_ERROR_REASON_NOT_LOADED,
    RESOURCE_ERROR_REASON_ALREADY_LOADED,
    RESOURCE_ERROR_REASON_NO_LOADER,
    RESOURCE_ERROR_REASON_LOAD_FAILED,
    RESOURCE_ERROR_REASON_MALFORMED_CONTENT,
    RESOURCE_ERROR_REASON_INVALID_CONTENT,
    RESOURCE_ERROR_REASON_UNSUPPORTED_CONTENT,
    RESOURCE_ERROR_REASON_UNEXPECTED_REFERENCE_TYPE,
} ResourceErrorReason;

typedef void *argus_resource_error_t;

argus_resource_prototype_t argus_resource_get_prototype(argus_resource_const_t resource);

void argus_resource_release(argus_resource_const_t resource);

const void *argus_resource_get_data_ptr(argus_resource_const_t resource);

argus_resource_error_t argus_resource_error_new(ResourceErrorReason reason, const char *uid, const char *info);

void argus_resource_error_destruct(argus_resource_error_t error);

ResourceErrorReason argus_resource_error_get_reason(argus_resource_error_t error);

const char *argus_resource_error_get_uid(argus_resource_error_t error);

const char *argus_resource_error_get_info(argus_resource_error_t error);

#ifdef __cplusplus
}
#endif

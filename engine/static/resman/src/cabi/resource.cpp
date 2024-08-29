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

#include "argus/resman/cabi/resource.h"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

using argus::Resource;
using argus::ResourceError;

static inline const Resource &_as_ref(argus_resource_const_t ptr) {
    return *reinterpret_cast<const Resource *>(ptr);
}

static inline ResourceError &_error_as_ref(argus_resource_error_t &error) {
    return *reinterpret_cast<ResourceError *>(error);
}

#ifdef __cplusplus
extern "C" {
#endif

argus_resource_prototype_t argus_resource_get_prototype(argus_resource_const_t resource) {
    const auto &cpp_proto = _as_ref(resource).prototype;
    argus_resource_prototype_t c_proto {
        cpp_proto.uid.c_str(),
        cpp_proto.media_type.c_str(),
        reinterpret_cast<const char *>(cpp_proto.fs_path.c_str()), // workaround for MSVC
    };
    return c_proto;
}

void argus_resource_release(argus_resource_const_t resource) {
    _as_ref(resource).release();
}

argus_resource_error_t argus_resource_error_new(ResourceErrorReason reason, const char *uid, const char *info) {
    return new argus::ResourceError { argus::ResourceErrorReason(reason), uid, info };
}

void argus_resource_error_destruct(argus_resource_error_t error) {
    delete &_error_as_ref(error);
}

const void *argus_resource_get_data_ptr(argus_resource_const_t resource) {
    return _as_ref(resource).get_data_ptr();
}

ResourceErrorReason argus_resource_error_get_reason(argus_resource_error_t error) {
    return ResourceErrorReason(_error_as_ref(error).reason);
}

const char *argus_resource_error_get_uid(argus_resource_error_t error) {
    return _error_as_ref(error).uid.c_str();
}

const char *argus_resource_error_get_info(argus_resource_error_t error) {
    return _error_as_ref(error).info.c_str();
}

#ifdef __cplusplus
}
#endif

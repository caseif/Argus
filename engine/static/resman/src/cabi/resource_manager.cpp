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

#include "argus/resman/cabi/resource_manager.h"

#include "argus/lowlevel/result.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#ifdef __cplusplus
extern "C" {
#endif

using argus::Result;

using argus::Resource;
using argus::ResourceError;
using argus::ResourceManager;

static inline ResourceManager &_as_ref(argus_resource_manager_t ptr) {
    return *reinterpret_cast<ResourceManager *>(ptr);
}

static inline const ResourceOrResourceError _wrap_result(const Result<Resource &, ResourceError> &res) {
        ResourceOrResourceError wrapped;
        wrapped.is_ok = res.is_ok();
    if (res.is_ok()) {
        wrapped.ve.value = &res.unwrap();
    } else {
        wrapped.ve.error = argus_resource_error_t { new ResourceError(res.unwrap_err()) };
    }
    return wrapped;
}

argus_resource_manager_t argus_resource_manager_get_instance(void) {
    return &ResourceManager::instance();
}

void argus_resource_manager_discover_resources(argus_resource_manager_t mgr) {
    _as_ref(mgr).discover_resources();
}

void argus_resource_manager_add_memory_package(argus_resource_manager_t mgr, const unsigned char *buf, size_t len) {
    _as_ref(mgr).add_memory_package(buf, len);
}

//TODO: register_loader

//TODO: register_extension_mappings

ResourceOrResourceError argus_resource_manager_get_resource(argus_resource_manager_t mgr, const char *uid) {
    return _wrap_result(_as_ref(mgr).get_resource(uid));
}

ResourceOrResourceError argus_resource_manager_get_resource_weak(argus_resource_manager_t mgr, const char *uid) {
    return _wrap_result(_as_ref(mgr).get_resource_weak(uid));
}

ResourceOrResourceError argus_resource_manager_try_get_resource(argus_resource_manager_t mgr, const char *uid) {
    return _wrap_result(_as_ref(mgr).try_get_resource(uid));
}

void argus_resource_manager_get_resource_async(argus_resource_manager_t mgr, const char *uid,
        void(*callback)(ResourceOrResourceError)) {
    _as_ref(mgr).get_resource_async(uid,
            [callback](Result<Resource &, argus::ResourceError> res) { callback(_wrap_result(res)); });
}

ResourceOrResourceError argus_resource_manager_create_resource(argus_resource_manager_t mgr, const char *uid,
        const char *media_type, const void *data, size_t len) {
    return _wrap_result(_as_ref(mgr).create_resource(uid, media_type, data, len));
}

#ifdef __cplusplus
}
#endif

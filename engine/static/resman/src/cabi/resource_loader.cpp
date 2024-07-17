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

#include "argus/resman/cabi/resource_loader.h"
#include "internal/resman/cabi/resource_loader.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/lowlevel/macros.hpp"

#include <istream>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

using argus::Resource;
using argus::ResourceError;
using argus::ResourceLoader;
using argus::ResourceManager;
using argus::ResourcePrototype;
using argus::Result;

typedef std::vector<std::pair<std::string, const Resource *>> LoadedDependencySetImpl;

static inline Result<void *, ResourceError> _unwrap_voidptr_result(VoidPtrOrResourceError res) {
    if (res.is_ok) {
        return argus::ok<void *, ResourceError>(res.ve.value);
    } else {
        ResourceError *unwrapped_err = reinterpret_cast<ResourceError *>(res.ve.error);
        ResourceError err_copy = *unwrapped_err;
        delete unwrapped_err;
        return argus::err<void *, ResourceError>(err_copy);
    }
}

static LoadedDependencySetOrResourceError _wrap_dep_result(
        Result<std::map<std::string, const Resource *>, ResourceError> res) {
    LoadedDependencySetOrResourceError wrapped_res;
    wrapped_res.is_ok = res.is_ok();

    if (res.is_ok()) {
        const auto &map = res.unwrap();
        wrapped_res.ve.value = new LoadedDependencySetImpl(map.begin(), map.end());
    } else {
        wrapped_res.ve.error = new ResourceError(res.unwrap_err());
    }

    return wrapped_res;
}

static bool _read_callback(void *dst, size_t len, void *engine_data) {
    auto &stream = *reinterpret_cast<std::istream *>(engine_data);
    stream.read(reinterpret_cast<char *>(dst), std::streamsize(len));
    return stream.good();
}

static const LoadedDependencySetImpl &_lds_as_ref(argus_loaded_dependency_set_t ptr) {
    return *reinterpret_cast<const LoadedDependencySetImpl *>(ptr);
}

ProxiedResourceLoader::ProxiedResourceLoader(
        std::vector<std::string> media_types,
        argus_resource_load_fn_t load_fn,
        argus_resource_copy_fn_t copy_fn,
        argus_resource_unload_fn_t unload_fn,
        void *user_data
):
    ResourceLoader(std::move(media_types)),
    m_load_fn(load_fn),
    m_copy_fn(copy_fn),
    m_unload_fn(unload_fn),
    m_user_data(user_data) {
}

Result<void *, ResourceError> ProxiedResourceLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
        std::istream &stream, size_t size) {
    auto wrapped_proto = argus_resource_prototype_t {
            proto.uid.c_str(),
            proto.media_type.c_str(),
            reinterpret_cast<const char *>(proto.fs_path.c_str()), // workaround for MSVC
    };
    return _unwrap_voidptr_result(m_load_fn(this, &manager, wrapped_proto, _read_callback, size,
            m_user_data, &stream));
}

Result<void *, ResourceError> ProxiedResourceLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
        void *src, std::type_index type) {
    UNUSED(type);
    auto wrapped_proto = argus_resource_prototype_t {
            proto.uid.c_str(),
            proto.media_type.c_str(),
            reinterpret_cast<const char *>(proto.fs_path.c_str()), // workaround for MSVC
    };
    return _unwrap_voidptr_result(m_copy_fn(this, &manager, wrapped_proto, src, m_user_data));
}

void ProxiedResourceLoader::unload(void *data_ptr) {
    m_unload_fn(this, data_ptr, m_user_data);
}

#ifdef __cplusplus
extern "C" {
#endif

argus_resource_loader_t argus_resource_loader_new(
        const char *const *media_types,
        size_t media_types_count,
        argus_resource_load_fn_t load_fn,
        argus_resource_copy_fn_t copy_fn,
        argus_resource_unload_fn_t unload_fn,
        void *user_data
) {
    return new ProxiedResourceLoader(std::vector<std::string>(media_types, media_types + media_types_count),
            load_fn, copy_fn, unload_fn, user_data);
}

LoadedDependencySetOrResourceError argus_resource_loader_load_dependencies(argus_resource_loader_t loader,
        argus_resource_manager_t manager, const char *const *dependencies, size_t dependencies_count) {
    auto &real_loader = *reinterpret_cast<ProxiedResourceLoader *>(loader);
    auto &real_mgr = *reinterpret_cast<ResourceManager *>(manager);
    return _wrap_dep_result(real_loader.load_dependencies(real_mgr,
            std::vector<std::string>(dependencies, dependencies + dependencies_count)));
}

size_t argus_loaded_dependency_set_get_count(argus_loaded_dependency_set_t set) {
    return _lds_as_ref(set).size();
}

const char *argus_loaded_dependency_set_get_name_at(argus_loaded_dependency_set_t set, size_t index) {
    return _lds_as_ref(set).at(index).first.c_str();
}

argus_resource_const_t argus_loaded_dependency_set_get_resource_at(argus_loaded_dependency_set_t set, size_t index) {
    return _lds_as_ref(set).at(index).second;
}

void argus_loaded_dependency_set_destruct(argus_loaded_dependency_set_t set) {
    delete &_lds_as_ref(set);
}

#ifdef __cplusplus
}
#endif

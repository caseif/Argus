/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/filesystem.hpp"
#include "argus/lowlevel/streams.hpp"
#include "argus/lowlevel/threading.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/module.hpp"

// module resman
#include "argus/resman/exception.hpp"
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource_loader.hpp"
#include "internal/resman/pimpl/resource_manager.hpp"

#include "arp/util/media_types.h"

#include <algorithm>
#include <exception> // IWYU pragma: keep
#include <functional>
#include <future>
#include <istream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <cctype>

#define UID_SEPARATOR "."

#define RESOURCES_DIR "resources"

namespace argus {
    ResourceManager g_global_resource_manager;

    void _update_lifecycle_resman(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::POST_INIT:
                g_global_resource_manager.discover_resources();
                break;
            default:
                break;
        }
    }

    static void _load_initial_ext_mappings(std::map<std::string, std::string> &target) {
        size_t count = 0;
        const extension_mapping_t *mappings = arp_get_extension_mappings(&count);

        if (count == 0) {
            return;
        }

        for (size_t i = 0; i < count; i++) {
            const extension_mapping_t *mapping = &mappings[i];
            target.insert({std::string(mapping->extension), std::string(mapping->media_type) });
        }
    }

    void init_module_resman(void) {
        register_module({MODULE_RESMAN, 2, {"core"}, _update_lifecycle_resman});
    }

    ResourceManager &ResourceManager::get_global_resource_manager(void) {
        return g_global_resource_manager;
    }

    ResourceManager::ResourceManager(void):
            pimpl(new pimpl_ResourceManager()) {
        _load_initial_ext_mappings(pimpl->extension_mappings);
    }

    ResourceManager::~ResourceManager(void) {
        delete pimpl;
    }

    int ResourceManager::unload_resource(const std::string &uid) {
        auto it = pimpl->loaded_resources.find(uid);
        if (it == pimpl->loaded_resources.cend()) {
            throw ResourceNotLoadedException(uid);
        }

        Resource *res = it->second;

        pimpl->loaded_resources.erase(uid);

        delete res;

        return 0;
    }

    static void _discover_fs_resources_recursively(const std::string &root_path, const std::string &prefix,
            std::map<std::string, ResourcePrototype> &prototype_map,
            const std::map<std::string, std::string> &extension_map) {
        std::vector<std::string> children;
        try {
            children = list_directory_entries(root_path);
        } catch (std::exception &ex) {
            _ARGUS_WARN("Failed to discover resources: %s\n", ex.what());
            return;
        }

        for (std::string child : children) {
            std::string full_child_path = root_path + PATH_SEPARATOR + child;

            auto name_and_ext = get_name_and_extension(child);
            std::string name = name_and_ext.first;
            std::string ext = name_and_ext.second;

            std::string cur_uid;
            if (prefix.empty()) {
                cur_uid = name;
            } else {
                cur_uid = prefix + UID_SEPARATOR + name;
            }

            if (is_directory(full_child_path)) {
                _discover_fs_resources_recursively(full_child_path, cur_uid, prototype_map, extension_map);
            } else if (is_regfile(full_child_path)) {
                if (ext.empty()) {
                    _ARGUS_WARN("Resource %s does not have an extension, ignoring\n", full_child_path.c_str());
                    continue;
                }

                if (prototype_map.find(cur_uid) != prototype_map.cend()) {
                    _ARGUS_WARN("Resource %s exists with multiple prefixes, ignoring further copies", cur_uid.c_str());
                    continue;
                }

                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

                auto type_it = extension_map.find(ext);
                if (type_it == extension_map.cend()) {
                    _ARGUS_WARN("Discovered filesystem resource %s with unknown extension %s, ignoring\n",
                            cur_uid.c_str(), ext.c_str());
                    continue;
                }

                prototype_map.insert({cur_uid, {cur_uid, type_it->second, full_child_path, true}});

                _ARGUS_DEBUG("Discovered filesystem resource %s at path %s\n", cur_uid.c_str(),
                        full_child_path.c_str());
            }
        }
    }

    void ResourceManager::discover_resources(void) {
        try {
        std::string exe_path = get_executable_path();

        std::string exe_dir = get_parent(exe_path);

        _discover_fs_resources_recursively(exe_dir + PATH_SEPARATOR + RESOURCES_DIR, "",
                pimpl->discovered_resource_prototypes, pimpl->extension_mappings);
        } catch (std::exception &ex) {
            _ARGUS_FATAL("Failed to get executable directory: %s\n", ex.what());
        }
    }

    void ResourceManager::register_loader(const std::string &media_type, ResourceLoader *const loader) {
        if (pimpl->registered_loaders.find(media_type) != pimpl->registered_loaders.cend()) {
            throw std::invalid_argument("Cannot register loader for type more than once");
        }

        pimpl->registered_loaders.insert({media_type, loader});
    }

    void ResourceManager::register_extension_mappings(const std::map<std::string, std::string> &mappings) {
        pimpl->extension_mappings.insert(mappings.begin(), mappings.end());
    }

    Resource &ResourceManager::get_resource(const std::string &uid) {
        if (pimpl->discovered_resource_prototypes.find(uid) == pimpl->discovered_resource_prototypes.cend()) {
            throw ResourceNotPresentException(uid);
        }

        auto it = pimpl->loaded_resources.find(uid);
        if (it != pimpl->loaded_resources.cend()) {
            return *it->second;
        } else {
            return load_resource(uid);
        }
    }

    Resource &ResourceManager::try_get_resource(const std::string &uid) const {
        if (pimpl->discovered_resource_prototypes.find(uid) == pimpl->discovered_resource_prototypes.cend()) {
            throw ResourceNotPresentException(uid);
        }

        auto it = pimpl->loaded_resources.find(uid);
        if (it == pimpl->loaded_resources.cend()) {
            throw ResourceNotLoadedException(uid);
        }

        return *it->second;
    }

    Resource &ResourceManager::load_resource(const std::string &uid) {
        if (pimpl->loaded_resources.find(uid) != pimpl->loaded_resources.cend()) {
            throw ResourceLoadedException(uid);
        }

        auto pt_it = pimpl->discovered_resource_prototypes.find(uid);
        if (pt_it == pimpl->discovered_resource_prototypes.cend()) {
            throw ResourceNotPresentException(uid);
        }

        ResourcePrototype proto = pt_it->second;

        if (!proto.fs_path.empty()) {
            FileHandle file_handle = FileHandle::create(proto.fs_path, FILE_MODE_READ);

            auto loader_it = pimpl->registered_loaders.find(proto.media_type);
            if (loader_it == pimpl->registered_loaders.end()) {
                throw NoLoaderException(uid, proto.media_type);
            }

            std::ifstream stream;
            file_handle.to_istream(0, stream);

            ResourceLoader *loader = loader_it->second;
            loader->pimpl->last_dependencies = {};
            void *const data_ptr = loader->load(proto, stream, file_handle.get_size());

            if (!data_ptr) {
                stream.close();
                file_handle.release();
                throw LoadFailedException(uid);
            }

            Resource *res = new Resource(*this, proto, data_ptr, loader->pimpl->last_dependencies);
            pimpl->loaded_resources.insert({proto.uid, res});

            stream.close();
            file_handle.release();

            return *res;
        } else {
            //TODO: read from resource package
            throw ResourceNotPresentException(uid);
        }
    }

    std::future<Resource&> ResourceManager::load_resource_async(const std::string &uid,
                    const std::function<void(Resource&)> callback) {
        return make_future<Resource&>(std::bind(&ResourceManager::load_resource, this, uid), callback);
    }

    std::future<Resource&> ResourceManager::load_resource_async(const std::string &uid) {
        return load_resource_async(uid, nullptr);
    }

    Resource &ResourceManager::create_resource(const std::string &uid, const std::string &media_type, const void *data,
            size_t len) {
        if (pimpl->loaded_resources.find(uid) != pimpl->loaded_resources.cend()) {
            throw ResourceLoadedException(uid);
        }
        
        auto loader_it = pimpl->registered_loaders.find(media_type);
        if (loader_it == pimpl->registered_loaders.end()) {
            throw NoLoaderException(uid, media_type);
        }

        MemIstream stream(data, len);

        ResourcePrototype proto = { uid, media_type, "", false };

        ResourceLoader *loader = loader_it->second;
        loader->pimpl->last_dependencies = {};
        void *const data_ptr = loader->load(proto, stream, len);

        if (!data_ptr) {
            throw LoadFailedException(uid);
        }

        Resource *res = new Resource(*this, proto, data_ptr, loader->pimpl->last_dependencies);
        pimpl->loaded_resources.insert({uid, res});

        return *res;
    }
}

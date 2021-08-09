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
#include "argus/core/engine.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"

// module resman
#include "argus/resman/exception.hpp"
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_event.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource.hpp"
#include "internal/resman/pimpl/resource_loader.hpp"
#include "internal/resman/pimpl/resource_manager.hpp"

#include "arp/arp.h"

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

#define UID_NS_SEPARATOR ':'
#define UID_PATH_SEPARATOR '/'

#define RESOURCES_DIR "resources"

namespace argus {
    ResourceManager g_global_resource_manager;

    static void _load_initial_ext_mappings(std::map<std::string, std::string> &target) {
        size_t count = 0;
        extension_mapping_t *mappings = arp_get_extension_mappings(&count);

        if (count == 0) {
            return;
        }

        for (size_t i = 0; i < count; i++) {
            const extension_mapping_t *mapping = &mappings[i];
            target.insert({std::string(mapping->extension), std::string(mapping->media_type) });
        }

        arp_free_extension_mappings(mappings);
    }

    ResourceManager &ResourceManager::get_global_resource_manager(void) {
        return g_global_resource_manager;
    }

    ResourceManager::ResourceManager(void):
            pimpl(new pimpl_ResourceManager()) {
        pimpl->package_set = arp_create_set();
        pimpl->discovery_done = false;

        for (auto &res : pimpl->loaded_resources) {
            this->unload_resource(res.first);
        }

        _load_initial_ext_mappings(pimpl->extension_mappings);
    }

    ResourceManager::~ResourceManager(void) {
        arp_unload_set_packages(pimpl->package_set);
        arp_destroy_set(pimpl->package_set);

        delete pimpl;
    }

    static void _discover_arp_packages(ArpPackageSet set, const std::string &root_path) {
        std::vector<std::string> children;
        try {
            children = list_directory_entries(root_path);
        } catch (std::exception &ex) {
            _ARGUS_WARN("Failed to discover resources: %s\n", ex.what());
            return;
        }

        for (const std::string &child : children) {
            std::string full_child_path = root_path + PATH_SEPARATOR + child;

            auto name_and_ext = get_name_and_extension(child);
            std::string name = name_and_ext.first;
            std::string ext = name_and_ext.second;

            if (ext != "arp") {
                continue;
            }

            //TODO: skip part files

            ArpPackage package = nullptr;
            int rc = 0;
            if ((rc = arp_load_from_file(full_child_path.c_str(), &package)) != 0) {
                _ARGUS_WARN("Failed to load package at path %s (libarp returned error code %d)\n",
                        full_child_path.c_str(), rc);
            }

            if ((rc = arp_add_to_set(set, package)) != 0) {
                _ARGUS_WARN("Failed to add package at path %s to set (libarp returned error code %d)\n",
                        full_child_path.c_str(), rc);
            }
        }
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

        for (const std::string &child : children) {
            std::string full_child_path = root_path + PATH_SEPARATOR + child;

            auto name_and_ext = get_name_and_extension(child);
            std::string name = name_and_ext.first;
            std::string ext = name_and_ext.second;

            std::string cur_uid;
            if (prefix.empty()) {
                if (is_regfile(full_child_path)) {
                    if (ext != "arp") {
                        _ARGUS_WARN("Ignoring non-namespaced filesystem resource %s\n", name.c_str());
                    }
                    continue;
                }

                cur_uid = name + UID_NS_SEPARATOR;
            } else {
                if (prefix.back() == UID_NS_SEPARATOR) {
                    cur_uid = prefix + name;
                } else {
                    cur_uid = prefix + UID_PATH_SEPARATOR + name;
                }
            }

            if (is_directory(full_child_path)) {
                _discover_fs_resources_recursively(full_child_path, cur_uid, prototype_map, extension_map);
            } else if (is_regfile(full_child_path)) {
                if (ext.empty()) {
                    _ARGUS_WARN("Resource %s does not have an extension, ignoring\n", full_child_path.c_str());
                    continue;
                }

                if (prototype_map.find(cur_uid) != prototype_map.cend()) {
                    _ARGUS_WARN("Resource %s exists with multiple prefixes, ignoring further copies\n", cur_uid.c_str());
                    continue;
                }

                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

                auto type_it = extension_map.find(ext);
                if (type_it == extension_map.cend()) {
                    _ARGUS_WARN("Discovered filesystem resource %s with unknown extension %s, ignoring\n",
                            cur_uid.c_str(), ext.c_str());
                    continue;
                }

                prototype_map.insert({cur_uid, {cur_uid, type_it->second, full_child_path}});

                _ARGUS_DEBUG("Discovered filesystem resource %s at path %s\n", cur_uid.c_str(),
                        full_child_path.c_str());
            }
        }
    }

    void ResourceManager::discover_resources(void) {
        try {
            std::string exe_path = get_executable_path();

            std::string exe_dir = get_parent(exe_path);

            std::string res_dir = exe_dir + PATH_SEPARATOR + RESOURCES_DIR;

            _discover_arp_packages(pimpl->package_set, res_dir);

            _discover_fs_resources_recursively(res_dir, "", pimpl->discovered_fs_protos, pimpl->extension_mappings);

            pimpl->discovery_done = true;
        } catch (std::exception &ex) {
            _ARGUS_FATAL("Failed to get executable directory: %s\n", ex.what());
        }
    }

    void ResourceManager::add_memory_package(const unsigned char *buf, size_t len) {
        int rc = 0;

        ArpPackage pack = nullptr;
        if ((rc = arp_load_from_memory(buf, len, &pack)) != 0) {
            _ARGUS_FATAL("Failed to load in-memory package (return code %d)\n", rc);
        }
        arp_add_to_set(pimpl->package_set, pack);
    }

    void ResourceManager::register_loader(ResourceLoader &loader) {
        for (const std::string &media_type : loader.pimpl->media_types) {
            if (pimpl->registered_loaders.find(media_type) != pimpl->registered_loaders.cend()) {
                throw std::invalid_argument("Cannot register loader for type more than once");
            }
            pimpl->registered_loaders.insert({media_type, &loader});
        }

    }

    void ResourceManager::register_extension_mappings(const std::map<std::string, std::string> &mappings) {
        pimpl->extension_mappings.insert(mappings.begin(), mappings.end());
    }

    static Resource *_acquire_resource(const ResourceManager &mgr, const std::string &uid, bool inc_refcount = true) {
        auto it = mgr.pimpl->loaded_resources.find(uid);
        if (it != mgr.pimpl->loaded_resources.cend()) {
            if (inc_refcount) {
                auto new_ref_count = it->second->pimpl->ref_count.fetch_add(1) + 1;
                _ARGUS_DEBUG("Acquired handle for resource %s (new refcount is %d)\n", uid.c_str(), new_ref_count);
            }

            return it->second;
        } else {
            return nullptr;
        }
    }

    Resource &ResourceManager::get_resource(const std::string &uid) {
        auto *res = _acquire_resource(*this, uid);
        if (res != nullptr) {
            return *res;
        } else {
            return load_resource(uid);
        }
    }

    Resource &ResourceManager::get_resource_weak(const std::string &uid) {
        auto *res = _acquire_resource(*this, uid, false);
        if (res != nullptr) {
            return *res;
        } else {
            throw ResourceNotLoadedException(uid);
        }
    }

    Resource &ResourceManager::try_get_resource(const std::string &uid) const {
        auto *res = _acquire_resource(*this, uid);
        if (res != nullptr) {
            return *res;
        } else {
            throw ResourceNotLoadedException(uid);
        }
    }

    Resource &ResourceManager::load_resource(const std::string &uid) {
        if (pimpl->loaded_resources.find(uid) != pimpl->loaded_resources.cend()) {
            throw ResourceLoadedException(uid);
        }

        _ARGUS_DEBUG("Initiating load for resource %s\n", uid.c_str());

        Resource *res = nullptr;

        auto pt_it = pimpl->discovered_fs_protos.find(uid);
        if (pt_it != pimpl->discovered_fs_protos.cend()) {
            ResourcePrototype proto = pt_it->second;

            _ARGUS_ASSERT(!proto.fs_path.empty(), "FS resource path is empty\n");

            FileHandle file_handle = FileHandle::create(proto.fs_path, FILE_MODE_READ);

            auto loader_it = pimpl->registered_loaders.find(proto.media_type);
            if (loader_it == pimpl->registered_loaders.end()) {
                throw NoLoaderException(uid, proto.media_type);
            }

            std::ifstream stream;
            file_handle.to_istream(0, stream);

            auto &loader = *loader_it->second;
            loader.pimpl->last_dependencies = {};
            void *const data_ptr = loader.load(*this, proto, stream, file_handle.get_size());

            stream.close();
            file_handle.release();

            if (data_ptr == nullptr) {
                throw LoadFailedException(uid);
            }

            res = new Resource(*this, loader, proto, data_ptr, loader.pimpl->last_dependencies);

            _ARGUS_DEBUG("Loaded filesystem resource %s of type %s\n", proto.uid.c_str(), proto.media_type.c_str());
        } else {
            arp_resource_meta_t res_meta = {};
            int rc = arp_find_resource_in_set(pimpl->package_set, uid.c_str(), &res_meta);

            if (rc != 0) {
                if (rc == E_ARP_RESOURCE_NOT_FOUND) {
                    throw ResourceNotPresentException(uid);
                } else {
                    throw LoadFailedException(uid);
                }
            }

            auto *arp_res = arp_load_resource(&res_meta);

            if (arp_res == nullptr) {
                throw LoadFailedException(uid);
            }

            ResourcePrototype proto(uid, res_meta.media_type, "");

            auto loader_it = pimpl->registered_loaders.find(proto.media_type);
            if (loader_it == pimpl->registered_loaders.end()) {
                throw NoLoaderException(uid, proto.media_type);
            }

            IMemStream stream(arp_res->data, arp_res->meta.size);

            auto &loader = *loader_it->second;
            loader.pimpl->last_dependencies = {};
            void *const data_ptr = loader.load(*this, proto, stream, arp_res->meta.size);

            if (!data_ptr) {
                throw LoadFailedException(uid);
            }

            arp_unload_resource(arp_res);

            res = new Resource(*this, loader, proto, data_ptr, loader.pimpl->last_dependencies);

            _ARGUS_DEBUG("Loaded ARP resource %s of type %s\n", proto.uid.c_str(), proto.media_type.c_str());
        }

        if (res == nullptr) {
            throw ResourceNotPresentException(uid);
        }

        pimpl->loaded_resources.insert({res->uid, res});

        dispatch_event<ResourceEvent>(ResourceEventType::Load, res->prototype, res);

        _ARGUS_DEBUG("Loaded resource %s (initial refcount is %d)\n",
                res->prototype.uid.c_str(), res->pimpl->ref_count.load());

        return *res;
    }

    std::future<Resource&> ResourceManager::load_resource_async(const std::string &uid,
                    const std::function<void(Resource&)> &callback) {
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

        IMemStream stream(data, len);

        ResourcePrototype proto = { uid, media_type, "" };

        auto &loader = *loader_it->second;
        loader.pimpl->last_dependencies = {};
        void *const data_ptr = loader.load(*this, proto, stream, len);

        if (!data_ptr) {
            throw LoadFailedException(uid);
        }

        Resource *res = new Resource(*this, loader, proto, data_ptr, loader.pimpl->last_dependencies);
        pimpl->loaded_resources.insert({uid, res});

        return *res;
    }

    int ResourceManager::unload_resource(const std::string &uid) {
        _ARGUS_DEBUG("Unloading resource %s\n", uid.c_str());

        auto it = pimpl->loaded_resources.find(uid);
        if (it == pimpl->loaded_resources.cend()) {
            throw ResourceNotLoadedException(uid);
        }

        auto *res = it->second;

        dispatch_event<ResourceEvent>(ResourceEventType::Unload, res->prototype, nullptr);

        pimpl->loaded_resources.erase(res->uid);

        res->pimpl->loader.unload(res->pimpl->data_ptr);

        for (auto &dep_uid : res->pimpl->dependencies) {
            this->get_resource_weak(dep_uid).release();
        }

        delete res;

        return 0;
    }
}

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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/filesystem.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/streams.hpp"
#include "argus/lowlevel/threading/future.hpp"

#include "argus/core/event.hpp"

#include "argus/resman/exception.hpp"
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_event.hpp"
#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/pimpl/resource.hpp"
#include "internal/resman/pimpl/resource_loader.hpp"
#include "internal/resman/pimpl/resource_manager.hpp"

#include "arp/unpack/load.h"
#include "arp/unpack/resource.h"
#include "arp/unpack/set.h"
#include "arp/unpack/types.h"
#include "arp/util/error.h"
#include "arp/util/media_types.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <future>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <cctype>
#include <cstddef>

#define UID_NS_SEPARATOR ':'
#define UID_PATH_SEPARATOR '/'

#define ARP_EXT "arp"

#define RESOURCES_DIR "resources"

namespace argus {
    static void _load_initial_ext_mappings(std::map<std::string, std::string> &target) {
        size_t count = 0;
        extension_mapping_t *mappings = arp_get_extension_mappings(&count);

        if (count == 0) {
            return;
        }

        for (size_t i = 0; i < count; i++) {
            const extension_mapping_t *mapping = &mappings[i];
            target.insert({ std::string(mapping->extension), std::string(mapping->media_type) });
        }

        arp_free_extension_mappings(mappings);
    }

    ResourceManager &ResourceManager::instance(void) {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager::ResourceManager(void):
        m_pimpl(new pimpl_ResourceManager()) {
        m_pimpl->package_set = arp_create_set();
        m_pimpl->discovery_done = false;

        _load_initial_ext_mappings(m_pimpl->extension_mappings);
    }

    ResourceManager::~ResourceManager(void) {
        /*for (auto &res : m_pimpl->loaded_resources) {
            this->unload_resource(res.first);
        }*/

        arp_unload_set_packages(m_pimpl->package_set);
        arp_destroy_set(m_pimpl->package_set);

        delete m_pimpl;
    }

    static void _discover_arp_packages(ArpPackageSet set, const std::filesystem::path &root_path) {
        std::vector<std::string> children;

        for (const auto &child : std::filesystem::directory_iterator(root_path)) {
            if (child.path().extension() != EXTENSION_SEPARATOR ARP_EXT) {
                continue;
            }

            //TODO: skip part files

            ArpPackage package = nullptr;
            int rc = 0;
            if ((rc = arp_load_from_file(child.path().string().c_str(), nullptr, &package)) != 0) {
                Logger::default_logger().warn("Failed to load package at path %s (libarp returned error code %d)",
                        child.path().c_str(), rc);
            }

            if ((rc = arp_add_to_set(set, package)) != 0) {
                Logger::default_logger().warn("Failed to add package at path %s to set (libarp returned error code %d)",
                        child.path().c_str(), rc);
            }
        }
    }

    static void _discover_fs_resources_recursively(const std::filesystem::path &root_path, const std::string &prefix,
            std::map<std::string, ResourcePrototype> &prototype_map,
            const std::map<std::string, std::string> &extension_map) {
        for (const auto &child : std::filesystem::directory_iterator(root_path)) {
            std::string cur_uid;
            if (prefix.empty()) {
                if (child.is_regular_file()) {
                    if (child.path().extension() != EXTENSION_SEPARATOR ARP_EXT) {
                        Logger::default_logger().warn("Ignoring non-namespaced filesystem resource %s",
                                child.path().stem().c_str());
                    }
                    continue;
                }

                cur_uid = child.path().stem().string() + UID_NS_SEPARATOR;
            } else {
                if (prefix.back() == UID_NS_SEPARATOR) {
                    cur_uid = prefix + child.path().stem().string();
                } else {
                    cur_uid = prefix + UID_PATH_SEPARATOR + child.path().stem().string();
                }
            }

            if (child.is_directory()) {
                _discover_fs_resources_recursively(child, cur_uid, prototype_map, extension_map);
            } else if (child.is_regular_file()) {
                if (!child.path().has_extension() || child.path().extension() == EXTENSION_SEPARATOR) {
                    Logger::default_logger().warn("Resource %s does not have an extension, ignoring",
                            child.path().c_str());
                    continue;
                }

                if (prototype_map.find(cur_uid) != prototype_map.cend()) {
                    Logger::default_logger().warn("Resource %s exists with multiple prefixes, ignoring further copies",
                            cur_uid.c_str());
                    continue;
                }

                auto ext = child.path().extension().string().substr(1);
                std::transform(ext.begin(), ext.end(), ext.begin(), [](auto c) {
                    return static_cast<unsigned char>(std::tolower(c));
                });

                auto type_it = extension_map.find(ext);
                if (type_it == extension_map.cend()) {
                    Logger::default_logger().warn(
                            "Discovered filesystem resource %s with unknown extension %s, ignoring",
                            cur_uid.c_str(), ext.c_str());
                    continue;
                }

                prototype_map.insert({ cur_uid, { cur_uid, type_it->second, child }});

                Logger::default_logger().debug("Discovered filesystem resource %s at path %s", cur_uid.c_str(),
                        child.path().c_str());
            }
        }
    }

    void ResourceManager::discover_resources(void) {
        try {
            auto res_dir = std::filesystem::current_path() / RESOURCES_DIR;

            Logger::default_logger().debug("Discovering ARP packages");

            _discover_arp_packages(m_pimpl->package_set, res_dir);

            Logger::default_logger().debug("Discovering loose resources from filesystem");

            _discover_fs_resources_recursively(res_dir, "", m_pimpl->discovered_fs_protos,
                    m_pimpl->extension_mappings);

            Logger::default_logger().debug("Resource discovery completed");

            m_pimpl->discovery_done = true;
        } catch (std::exception &ex) {
            Logger::default_logger().fatal("Failed to get executable directory: %s", ex.what());
        }
    }

    void ResourceManager::add_memory_package(const unsigned char *buf, size_t len) {
        int rc = 0;

        ArpPackage pack = nullptr;
        if ((rc = arp_load_from_memory(buf, len, nullptr, &pack)) != 0) {
            Logger::default_logger().fatal("Failed to load in-memory package (return code %d)", rc);
        }
        arp_add_to_set(m_pimpl->package_set, pack);

        Logger::default_logger().debug("Successfully loaded ARP package from memory with length %lu", len);
    }

    void ResourceManager::register_loader(ResourceLoader &loader) {
        Logger::default_logger().debug("Registering new resource loader");

        for (const std::string &media_type : loader.m_pimpl->media_types) {
            if (m_pimpl->registered_loaders.find(media_type) != m_pimpl->registered_loaders.cend()) {
                throw std::invalid_argument("Cannot register loader for type more than once");
            }
            m_pimpl->registered_loaders.insert({ media_type, &loader });
            Logger::default_logger().debug("Registered loader for media type %s", media_type.c_str());
        }

    }

    void ResourceManager::register_extension_mappings(const std::map<std::string, std::string> &mappings) {
        Logger::default_logger().debug("Registering resource extension mappings");
        m_pimpl->extension_mappings.insert(mappings.begin(), mappings.end());
    }

    static Resource *_acquire_resource(const ResourceManager &mgr, const std::string &uid, bool inc_refcount = true) {
        auto it = mgr.m_pimpl->loaded_resources.find(uid);
        if (it != mgr.m_pimpl->loaded_resources.cend()) {
            if (inc_refcount) {
                auto new_ref_count = it->second->m_pimpl->ref_count.fetch_add(1) + 1;
                Logger::default_logger().debug("Acquired handle for resource %s (new refcount is %d)", uid.c_str(),
                        new_ref_count);
                UNUSED(new_ref_count); // suppress unused warning in release configuration
            }

            return it->second;
        } else {
            return nullptr;
        }
    }

    Resource &ResourceManager::get_resource(const std::string &uid) {
        auto *existing_res = _acquire_resource(*this, uid);
        if (existing_res != nullptr) {
            return *existing_res;
        }

        Logger::default_logger().debug("Initiating load for resource %s", uid.c_str());

        Resource *res = nullptr;

        auto pt_it = m_pimpl->discovered_fs_protos.find(uid);
        if (pt_it != m_pimpl->discovered_fs_protos.cend()) {
            ResourcePrototype proto = pt_it->second;

            affirm_precond(!proto.fs_path.empty(), "FS resource path is empty");

            FileHandle file_handle = FileHandle::create(proto.fs_path, FILE_MODE_READ);

            Logger::default_logger().debug("Trying to get loader for media type %s", proto.media_type.c_str());

            auto loader_it = m_pimpl->registered_loaders.find(proto.media_type);
            if (loader_it == m_pimpl->registered_loaders.end()) {
                throw NoLoaderException(uid, proto.media_type);
            }

            std::ifstream stream;
            file_handle.to_istream(0, stream);

            auto &loader = *loader_it->second;
            loader.m_pimpl->last_dependencies = {};
            void *const data_ptr = loader.load(*this, proto, stream, file_handle.get_size());

            stream.close();
            file_handle.release();

            if (data_ptr == nullptr) {
                throw LoadFailedException(uid);
            }

            res = new Resource(*this, loader, proto, data_ptr, loader.m_pimpl->last_dependencies);

            Logger::default_logger().debug("Loaded filesystem resource %s of type %s", proto.uid.c_str(),
                    proto.media_type.c_str());
        } else {
            arp_resource_meta_t res_meta = {};
            int rc = arp_find_resource_in_set(m_pimpl->package_set, uid.c_str(), &res_meta);

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

            auto loader_it = m_pimpl->registered_loaders.find(proto.media_type);
            if (loader_it == m_pimpl->registered_loaders.end()) {
                throw NoLoaderException(uid, proto.media_type);
            }

            IMemStream stream(arp_res->data, arp_res->meta.size);

            auto &loader = *loader_it->second;
            loader.m_pimpl->last_dependencies = {};
            void *const data_ptr = loader.load(*this, proto, stream, arp_res->meta.size);

            if (!data_ptr) {
                throw LoadFailedException(uid);
            }

            arp_unload_resource(arp_res);

            res = new Resource(*this, loader, proto, data_ptr, loader.m_pimpl->last_dependencies);

            Logger::default_logger().debug("Loaded ARP resource %s of type %s", proto.uid.c_str(),
                    proto.media_type.c_str());
        }

        m_pimpl->loaded_resources.insert({ res->uid, res });

        dispatch_event<ResourceEvent>(ResourceEventType::Load, res->prototype, res);

        Logger::default_logger().debug("Loaded resource %s (initial refcount is %d)",
                res->prototype.uid.c_str(), res->m_pimpl->ref_count.load());

        return *res;
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

    std::future<Resource &> ResourceManager::get_resource_async(const std::string &uid,
            const std::function<void(Resource &)> &callback) {
        auto *res = _acquire_resource(*this, uid);
        if (res != nullptr) {
            auto promise_ptr = std::make_shared<std::promise<Resource &>>();
            std::future<Resource &> future = promise_ptr->get_future();
            promise_ptr->set_value(*res);
            return future;
        } else {
            return make_future<Resource &>([this, uid](void) -> auto & { return get_resource(uid); }, callback);
        }
    }

    Resource &ResourceManager::create_resource(const std::string &uid, const std::string &media_type, const void *data,
            size_t len) {
        Logger::default_logger().debug("Creating ad hoc resource %s (%zu bytes)", uid.c_str(), len);

        if (m_pimpl->loaded_resources.find(uid) != m_pimpl->loaded_resources.cend()) {
            throw ResourceLoadedException(uid);
        }

        auto loader_it = m_pimpl->registered_loaders.find(media_type);
        if (loader_it == m_pimpl->registered_loaders.end()) {
            throw NoLoaderException(uid, media_type);
        }

        IMemStream stream(data, len);

        ResourcePrototype proto = { uid, media_type, "" };

        auto &loader = *loader_it->second;
        loader.m_pimpl->last_dependencies = {};
        void *const data_ptr = loader.load(*this, proto, stream, len);

        if (!data_ptr) {
            throw LoadFailedException(uid);
        }

        Resource *res = new Resource(*this, loader, proto, data_ptr, loader.m_pimpl->last_dependencies);
        m_pimpl->loaded_resources.insert({ uid, res });

        Logger::default_logger().debug("Created ad hoc resource %s", uid.c_str());

        return *res;
    }

    Resource &ResourceManager::create_resource(const std::string &uid, const std::string &media_type, void *obj,
            std::type_index type) {
        Logger::default_logger().debug("Creating ad hoc resource %s", uid.c_str());

        if (m_pimpl->loaded_resources.find(uid) != m_pimpl->loaded_resources.cend()) {
            throw ResourceLoadedException(uid);
        }

        auto loader_it = m_pimpl->registered_loaders.find(media_type);
        if (loader_it == m_pimpl->registered_loaders.end()) {
            throw NoLoaderException(uid, media_type);
        }

        ResourcePrototype proto = { uid, media_type, "" };

        auto &loader = *loader_it->second;
        loader.m_pimpl->last_dependencies = {};
        void *const data_ptr = loader.copy(*this, proto, obj, type);

        if (!data_ptr) {
            throw LoadFailedException(uid);
        }

        Resource *res = new Resource(*this, loader, proto, data_ptr, loader.m_pimpl->last_dependencies);
        m_pimpl->loaded_resources.insert({ uid, res });

        Logger::default_logger().debug("Created ad hoc resource %s", uid.c_str());

        return *res;
    }

    int ResourceManager::unload_resource(const std::string &uid) {
        Logger::default_logger().debug("Unloading resource %s", uid.c_str());

        auto it = m_pimpl->loaded_resources.find(uid);
        if (it == m_pimpl->loaded_resources.cend()) {
            throw ResourceNotLoadedException(uid);
        }

        auto *res = it->second;

        dispatch_event<ResourceEvent>(ResourceEventType::Unload, res->prototype, nullptr);

        m_pimpl->loaded_resources.erase(res->uid);

        res->m_pimpl->loader.unload(res->m_pimpl->data_ptr);

        for (auto &dep_uid : res->m_pimpl->dependencies) {
            this->get_resource_weak(dep_uid).release();
        }

        delete res;

        return 0;
    }
}

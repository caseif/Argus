// module lowlevel
#include "argus/error.hpp"
#include "argus/filesystem.hpp"
#include "argus/threading.hpp"
#include "internal/logging.hpp"

// module core
#include "argus/core.hpp"

// module resman
#include "argus/resource_manager.hpp"

#include <algorithm>
#include <cctype>

#define UID_SEPARATOR "."

#define RESOURCES_DIR "resources"

namespace argus {

    ResourceManager g_global_resource_manager;

    void update_lifecycle_resman(LifecycleStage stage) {
        if (stage == LATE_INIT) {
            g_global_resource_manager.discover_resources();
        }
    }

    int ResourceLoader::load_dependencies(std::initializer_list<std::string> dependencies) {
        std::vector<Resource*> acquired;

        bool failed = false;
        auto it = dependencies.begin();
        for (it; it < dependencies.end(); it++) {
            auto res = g_global_resource_manager.loaded_resources.find(*it);
            if (res != g_global_resource_manager.loaded_resources.end()) {
                failed = true;
                break;
            }
            acquired.insert(acquired.begin(), res->second);
        }

        if (failed) {
            for (Resource *res : acquired) {
                res->release();
            }

            return -1;
        }

        last_dependencies = dependencies;

        return 0;
    }

    ResourceLoader::ResourceLoader(std::initializer_list<std::string> types,
            std::initializer_list<std::string> extensions):
            types(types),
            extensions(extensions) {
    }

    void const *const ResourceLoader::load(std::istream const &stream) const {
        return nullptr;
    }

    void ResourceLoader::unload(void *const data_ptr) const {
    }

    int ResourceManager::register_loader(std::string const &type_id, ResourceLoader const &loader) {
        if (registered_loaders.find(type_id) != registered_loaders.cend()) {
            set_error("Cannot register loader for type more than once");
            return -1;
        }

        registered_loaders.insert({type_id, new ResourceLoader(loader)});
        return 0;
    }

    static void _discover_fs_resources_recursively(std::string const &root_path, std::string const &prefix,
            std::map<std::string, ResourcePrototype> &prototype_map,
            std::map<std::string, std::string> const &extension_map) {
        std::vector<std::string> children;
        if (list_directory_files(root_path, &children) != 0) {
            _ARGUS_WARN("Failed to discover resources: %s\n", get_error().c_str());
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
                    _ARGUS_WARN("Resource %s exists with multiple prefixes, ignoring further copies",
                            cur_uid.c_str());
                    continue;
                }

                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){return std::tolower(c);});

                auto type_it = extension_map.find(ext);
                if (type_it == extension_map.cend()) {
                    _ARGUS_WARN("Discovered filesystem resource %s with unknown extension %s, ignoring\n",
                            cur_uid.c_str(), ext.c_str());
                    continue;
                }

                prototype_map.insert({cur_uid, {cur_uid, type_it->second, full_child_path}});

                _ARGUS_DEBUG("Discovered filesystem resource %s at path %s\n", cur_uid.c_str(), full_child_path.c_str());
            }
        }
    }

    void ResourceManager::discover_resources(void) {
        std::string exe_path;
        if (get_executable_path(&exe_path) != 0) {
            _ARGUS_FATAL("Failed to get executable directory: %s\n", get_error().c_str());
        }

        std::string exe_dir = get_parent(exe_path);

        _discover_fs_resources_recursively(exe_dir + PATH_SEPARATOR + RESOURCES_DIR, "",
                discovered_resource_prototypes, extension_registrations);
    }

    int ResourceManager::get_resource(std::string const &uid, Resource **target) {
        if (discovered_resource_prototypes.find(uid) == discovered_resource_prototypes.cend()) {
            set_error("Resource does not exist");
            return -1;
        }

        int rc = load_resource(uid);
        if (rc != 0) {
            return rc;
        }

        auto it = loaded_resources.find(uid);
        _ARGUS_ASSERT(it != loaded_resources.cend(), "Failed to get handle to loaded resource %s\n", uid.c_str());
        
        *target = it->second;

        return 0;
    }

    int ResourceManager::try_get_resource(std::string const &uid, Resource **const target) const {
        auto it = loaded_resources.find(uid);
        if (it == loaded_resources.cend()) {
            set_error("Resource is not loaded");
            return -1;
        }

        *target = it->second;
        return 0;
    }

    int ResourceManager::load_resource(std::string const &uid) {
        if (loaded_resources.find(uid) != loaded_resources.cend()) {
            return 0;
        }

        auto pt_it = discovered_resource_prototypes.find(uid);
        if (pt_it == discovered_resource_prototypes.cend()) {
            set_error("Resource does not exist");
            return -1;
        }
        ResourcePrototype proto = pt_it->second;

        if (!proto.fs_path.empty()) {
            FileHandle file_handle;
            int rc = FileHandle::create(proto.fs_path, FILE_MODE_READ, &file_handle);
            if (rc != 0) {
                return rc;
            }

            auto loader_it = registered_loaders.find(proto.type_id);
            if (loader_it == registered_loaders.end()) {
                set_error("No registered loader for type");
                return -1;
            }

            std::ifstream stream;
            file_handle.to_istream(0, &stream);

            ResourceLoader *loader = loader_it->second;
            loader->last_dependencies = {};
            void const *data_ptr = loader->load(stream);

            Resource *res = new Resource(*this, proto, data_ptr, loader->last_dependencies);
            loaded_resources.insert({proto.uid, res});

            stream.close();

            return 0;
        } else {
            //TODO: read from resource package
            return 0;
        }
    }

    void ResourceManager::load_resource_trampoline(AsyncResourceRequestHandle &handle) {
        bool res = load_resource(handle.data) == 0;
        handle.set_result(res);
    }
    
    AsyncResourceRequestHandle ResourceManager::load_reosurce_async(std::string const &uid,
            const AsyncResourceRequestCallback callback) {
        AsyncResourceRequestHandle handle = AsyncResourceRequestHandle(this, uid, callback);
        
        handle.execute(std::bind(&ResourceManager::load_resource_trampoline, this, std::placeholders::_1));

        return handle;
    }

    int ResourceManager::unload_resource(std::string const &uid) {
        auto it = loaded_resources.find(uid);
        if (it == loaded_resources.cend()) {
            set_error("Resource not loaded");
            return -1;
        }

        Resource *res = it->second;

        loaded_resources.erase(uid);

        delete res;

        return 0;
    }

    Resource::Resource(ResourceManager &manager, const ResourcePrototype prototype, const void *const data_ptr,
            std::vector<std::string> &dependencies):
            manager(manager),
            prototype(prototype),
            data_ptr(data_ptr),
            dependencies(dependencies),
            ref_count(0) {
    }

    Resource::Resource(Resource &rhs):
            manager(rhs.manager),
            prototype(rhs.prototype),
            data_ptr(rhs.data_ptr),
            ref_count(rhs.ref_count.load()) {
    }
    
    Resource::Resource(Resource &&rhs):
            manager(rhs.manager),
            prototype(std::move(rhs.prototype)),
            data_ptr(rhs.data_ptr),
            ref_count(rhs.ref_count.load()) {
    }

    void Resource::release(void) {
        if (--ref_count == 0) {
            manager.unload_resource(prototype.uid);
        }
    }

    ResourceManager &get_global_resource_manager(void) {
        return g_global_resource_manager;
    }

}

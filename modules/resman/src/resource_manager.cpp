// module lowlevel
#include "argus/error.hpp"
#include "argus/filesystem.hpp"
#include "internal/logging.hpp"

// module resman
#include "argus/resource_manager.hpp"

#define UID_SEPARATOR "."

namespace argus {

    ResourceManager g_global_resource_manager;

    void init_module_resman(void) {
        g_global_resource_manager.discover_resources();
    }

    template <typename ResourceDataType>
    Resource<ResourceDataType>::Resource(ResourceManager const &manager, const std::string uid):
            manager(manager),
            uid(uid),
            ready(false),
            ref_count(0) {
    }

    template <typename ResourceDataType>
    const bool Resource<ResourceDataType>::is_ready(void) const {
        return ready;
    }

    template <typename ResourceDataType>
    ResourceDataType const &Resource<ResourceDataType>::get_data(void) const {
        if (!ready) {
            _ARGUS_WARN("get_data called on non-ready resource with UID %s\n", uid);
        }

        return data;
    }

    template <typename ResourceDataType>
    void Resource<ResourceDataType>::release(void) {
        if (--ref_count == 0) {
            manager.unload_resource(*this);
            std::string uid = this->uid;
        }
    }

    template <typename ResourceDataType>
    int ResourceLoader<ResourceDataType>::load_dependencies(std::initializer_list<std::string> dependencies) {
        //TODO
        return 0;
    }

    template <typename ResourceDataType>
    int ResourceManager::register_loader(std::string const &type_id, const ResourceLoader<ResourceDataType> loader) {
        if (registered_loaders.find(type_id) != registered_loaders.cend()) {
            set_error("Cannot register loader for type more than once");
            return -1;
        }

        registered_loaders.insert(type_id, loader);
        return 0;
    }

    static void _discover_resources_recursively(std::string const &root_path, std::string const &prefix,
            std::map<const std::string, const ResourcePrototype> &prototype_map) {
        std::vector<std::string> children = list_directory_files(root_path);
        for (std::string child : children) {
            std::string full_child_path = root_path + PATH_SEPARATOR + child;

            std::string cur_uid;
            if (prefix.empty()) {
                cur_uid = child;
            } else {
                cur_uid = prefix + UID_SEPARATOR + child;
            }

            if (is_directory(full_child_path)) {
                _discover_resources_recursively(full_child_path, cur_uid, prototype_map);
            } else if (is_regfile(full_child_path)) {
                std::pair<std::string, std::string> name_and_ext = get_name_and_extension(child);
                if (name_and_ext.second.empty()) {
                    _ARGUS_WARN("Resource %s does not have an extension, ignoring\n", full_child_path.c_str());
                    continue;
                }

                if (prototype_map.find(name_and_ext.first) != prototype_map.cend()) {
                    _ARGUS_WARN("Resource %s exists with multiple prefixes, ignoring further copies",
                            cur_uid.c_str());
                    continue;
                }

                //TODO: type lookup
                prototype_map.insert({cur_uid, {cur_uid, name_and_ext.second}});
            }
        }
    }

    void ResourceManager::discover_resources(void) {
        std::string exe_path;
        if (get_executable_path(&exe_path) != 0) {
            _ARGUS_FATAL("Failed to get executable directory: %s\n", get_error().c_str());
        }

        std::string exe_dir = get_parent(exe_path);

        _discover_resources_recursively(exe_dir, "", discovered_resource_prototypes);
    }

    template <typename ResourceDataType>
    int ResourceManager::get_resource(std::string const &uid, Resource<ResourceDataType> **target) {
        if (discovered_resource_prototypes.find(uid) == discovered_resource_prototypes.cend()) {
            set_error("Resource does not exist");
            return -1;
        }

        int rc = load_resource(uid);
        if (rc != 0) {
            return rc;
        }

        *target = loaded_resources.find(uid);

        return 0;
    }

    template <typename ResourceDataType>
    int ResourceManager::try_get_resource(std::string const &uid, Resource<ResourceDataType> **const target) const {
        auto it = loaded_resources.find(uid);
        if (it == loaded_resources.cend()) {
            set_error("Resource is not loaded");
            return -1;
        }

        *target = it->second;
        return 0;
    }

    int ResourceManager::load_resource(std::string const &uid) {
        //TODO
        return 0;
    }

    void ResourceManager::load_resource_trampoline(AsyncResourceRequestHandle &handle) {
        handle.set_result(load_resource(handle.data) == 0);
    }

    AsyncResourceRequestHandle ResourceManager::load_reosurce_async(std::string const &uid,
            const AsyncResourceRequestCallback callback) {
        AsyncResourceRequestHandle handle = AsyncResourceRequestHandle(this, uid, callback);
        
        handle.execute(std::bind(&ResourceManager::load_resource_trampoline, this, std::placeholders::_1));

        return handle;
    }

    int ResourceManager::unload_resource(std::string const &uid) {
        loaded_resources.erase(uid);
        return 0;
    }

    ResourceManager &get_global_resource_manager(void) {
        return g_global_resource_manager;
    }

}

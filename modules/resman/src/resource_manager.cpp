// module lowlevel
#include "argus/error.hpp"
#include "internal/logging.hpp"

// module resman
#include "argus/resource_manager.hpp"

namespace argus {

    void init_module_resman(void) {
        //TODO: discover resources
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
    int ResourceManager::register_loader(const std::string type_id, ResourceLoader<ResourceDataType> loader) {
        auto it = registered_loaders.find(type_id);
        if (it != registered_loaders.cend()) {
            set_error("Cannot register loader for type more than once");
            return -1;
        }

        registered_loaders.insert(type_id, loader);
        return 0;
    }

    template <typename ResourceDataType>
    int ResourceManager::get_resource(const std::string uid, Resource<ResourceDataType> **target) {
        auto it = discovered_resource_prototypes.find(uid);
        if (it == discovered_resource_prototypes.cend()) {
            set_error("Resource does not exist");
            return -1;
        }

        ResourcePrototype prototype = it->second;
        //TODO
        return 0;
    }

    template <typename ResourceDataType>
    int ResourceManager::try_get_resource(const std::string uid, Resource<ResourceDataType> **target) const {
        auto it = loaded_resources.find(uid);
        if (it == loaded_resources.cend()) {
            set_error("Resource is not loaded");
            return -1;
        }

        *target = it->second;
        return 0;
    }

    template <typename ResourceDataType>
    int ResourceManager::load_resource(const std::string uid) {
        //TODO
        return 0;
    }

    template <typename ResourceDataType>
    int ResourceManager::load_reosurce_async(const std::string uid, AsyncResourceRequestHandle *handle,
            AsyncResourceRequestCallback callback) {
        //TODO
        return 0;
    }

    int ResourceManager::unload_resource(std::string const &uid) {
        loaded_resources.erase(uid);
        return 0;
    }

}

// module lowlevel
#include "argus/resource_manager.hpp"

// module core
#include "internal/core_util.hpp"

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
    }

    template <typename ResourceDataType>
    int ResourceManager::register_loader(const std::string type_id, ResourceLoader<ResourceDataType> loader) {
        auto it = registered_loaders.find(type_id);
        if (it != registered_loaders.cend()) {
            _ARGUS_WARN("Cannot register loader for type \"%s\" more than once\n", type_id);
            return -1;
        }

        registered_loaders.insert(type_id, loader);
        return 0;
    }

    template <typename ResourceDataType>
    int ResourceManager::get_resource(const std::string uid, Resource<ResourceDataType> **target) {
        //TODO
    }

    template <typename ResourceDataType>
    int ResourceManager::try_get_resource(const std::string uid, Resource<ResourceDataType> **target) const {
        auto it = loaded_resources.find(uid);
        if (it == loaded_resources.cend()) {
            return -1;
        }

        *target = it->second;
        return 0;
    }

    template <typename ResourceDataType>
    int ResourceManager::load_resource(const std::string uid) {
        //TODO
    }

    template <typename ResourceDataType>
    int ResourceManager::load_reosurce_async(const std::string uid, AsyncResourceRequestHandle *handle,
            AsyncResourceRequestCallback callback) {
        //TODO
    }

    int ResourceManager::unload_resource(std::string const &uid) {
        loaded_resources.erase(uid);
        return 0;
    }

}

#include "argus/resource_manager.hpp"

namespace argus {

    int ResourceLoader::load_dependencies(std::initializer_list<std::string> dependencies) {
        std::vector<Resource*> acquired;

        bool failed = false;
        auto it = dependencies.begin();
        for (it; it < dependencies.end(); it++) {
            auto res = ResourceManager::get_global_resource_manager().loaded_resources.find(*it);
            if (res != ResourceManager::get_global_resource_manager().loaded_resources.end()) {
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

    ResourceLoader::ResourceLoader(std::string type_id,
            std::initializer_list<std::string> extensions):
            type_id(type_id),
            extensions(extensions) {
        for (std::string ext : extensions) {
            ResourceManager::get_global_resource_manager().extension_registrations.insert({ext, type_id});
        }
    }

    void *const ResourceLoader::load(std::istream &stream, const size_t size) const {
        return nullptr;
    }

    void ResourceLoader::unload(void *const data_ptr) const {
    }

}

#include "argus/resource_manager.hpp"

namespace argus {

    extern ResourceManager g_global_resource_manager;

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

}

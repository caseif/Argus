#pragma once

#include "argus/threading.hpp"

#include <atomic>
#include <iostream>
#include <map>
#include <vector>

namespace argus {

    class ResourceManager;

    typedef AsyncRequestCallback<ResourceManager*, const std::string> AsyncResourceRequestCallback;
    typedef AsyncRequestHandle<ResourceManager*, const std::string> AsyncResourceRequestHandle;
    
    struct ResourcePrototype {
        std::string uid;
        std::string type_id;
    };

    template <typename ResourceDataType>
    class Resource {
        private:
            ResourceManager &manager;

            std::atomic_bool ready;
            std::atomic<unsigned int> ref_count;

            std::vector<std::string> dependencies;
            ResourceDataType data;

            Resource(ResourceManager const &manager, const std::string uid);

        public:
            const ResourcePrototype prototype;

            // god this is such a hack
            const struct {
                inline operator std::string() {
                    return prototype.uid;
                }
            } uid;
            const struct {
                inline operator std::string() {
                    return prototype.type_id;
                }
            } type_id;

            const bool is_ready(void) const;

            ResourceDataType const &get_data(void) const;

            void release(void);
    };

    template <typename ResourceDataType>
    class ResourceLoader {
        friend class ResourceManager;

        protected:
            int load_dependencies(std::initializer_list<std::string> dependencies);

            ResourceDataType load(std::istream stream);
    };

    class ResourceManager {
        private:
            std::map<const std::string, void*> discovered_resource_prototypes;
            std::map<const std::string, void*> loaded_resources;
            std::map<const std::string, void*> registered_loaders;

            int unload_resource(std::string const &uid);

            void load_resource_trampoline(AsyncResourceRequestHandle &handle);

        public:
            template <typename ResourceDataType>
            int register_loader(std::string const &type_id, const ResourceLoader<ResourceDataType> loader);

            template <typename ResourceDataType>
            int get_resource(std::string const &uid, Resource<ResourceDataType> **const target);

            template <typename ResourceDataType>
            int try_get_resource(std::string const &uid, Resource<ResourceDataType> **const target) const;

            int load_resource(std::string const &uid);

            AsyncResourceRequestHandle load_reosurce_async(std::string const &uid,
                    const AsyncResourceRequestCallback callback);
    };

}

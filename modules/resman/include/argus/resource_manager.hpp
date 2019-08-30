#pragma once

#include "argus/threading.hpp"

#include <atomic>
#include <iostream>
#include <map>
#include <vector>

namespace argus {

    class Resource;
    class ResourceManager;

    typedef AsyncRequestCallback<ResourceManager*, const std::string> AsyncResourceRequestCallback;
    typedef AsyncRequestHandle<ResourceManager*, const std::string> AsyncResourceRequestHandle;

    struct ResourcePrototype {
        std::string uid;
        std::string type_id;
        std::string fs_path;
    };

    class ResourceLoader {
        friend class ResourceManager;

        private:
            const std::string type_id;
            const std::vector<std::string> extensions;
            
            std::vector<std::string> last_dependencies;

        protected:
            ResourceLoader(std::string type_id, std::initializer_list<std::string> extensions);

            int load_dependencies(std::initializer_list<std::string> dependencies);

            virtual void const *const load(std::istream const &stream) const;

            virtual void unload(void *const data_ptr) const;
    };

    class ResourceManager {
        friend class ResourceLoader;
        friend class Resource;

        private:
            std::map<std::string, ResourcePrototype> discovered_resource_prototypes;
            std::map<std::string, Resource*> loaded_resources;

            std::map<std::string, ResourceLoader*> registered_loaders;
            std::map<std::string, std::string> extension_registrations;

            int unload_resource(std::string const &uid);

            void load_resource_trampoline(AsyncResourceRequestHandle &handle);

        public:
            void discover_resources(void);

            int register_loader(std::string const &type_id, ResourceLoader const &loader);

            int get_resource(std::string const &uid, Resource **const target);

            int try_get_resource(std::string const &uid, Resource **const target) const;

            int load_resource(std::string const &uid);

            AsyncResourceRequestHandle load_reosurce_async(std::string const &uid,
                    const AsyncResourceRequestCallback callback);
    };

    class Resource {
        friend class ResourceManager;

        private:
            ResourceManager &manager;

            std::atomic<unsigned int> ref_count;

            std::vector<std::string> dependencies;

            const void *const data_ptr;

            Resource(ResourceManager &manager, const ResourcePrototype prototype, const void *const data,
                    std::vector<std::string> &dependencies);

        public:
            const ResourcePrototype prototype;

            // god this is such a hack
            const struct {
                Resource &parent;
                inline operator std::string() {
                    return parent.prototype.uid;
                }
            } uid {*this};
            const struct {
                Resource &parent;
                inline operator std::string() {
                    return parent.prototype.type_id;
                }
            } type_id {*this};

            Resource(Resource &rhs);
            
            Resource(Resource &&rhs);

            void release(void);

            template <typename DataType>
            const DataType &get_data(void) {
                return *static_cast<DataType*>(data_ptr);
            }
    };

    ResourceManager &get_global_resource_manager(void);

}

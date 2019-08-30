#pragma once

#include "argus/threading.hpp"

#include <atomic>
#include <iostream>
#include <map>
#include <vector>

namespace argus {

    class ResourceParent;
    template <typename ResourceDataType>
    class Resource;
    class ResourceManager;

    typedef AsyncRequestCallback<ResourceManager*, const std::string> AsyncResourceRequestCallback;
    typedef AsyncRequestHandle<ResourceManager*, const std::string> AsyncResourceRequestHandle;

    struct ResourcePrototype {
        std::string uid;
        std::string type_id;
        std::string fs_path;
    };

    class ResourceLoaderParent {
        friend class ResourceManager;

        private:
            const std::vector<std::string> types;
            const std::vector<std::string> extensions;

        protected:
            ResourceLoaderParent(std::initializer_list<std::string> types, std::initializer_list<std::string> extensions);

            int load_dependencies(std::initializer_list<std::string> dependencies);
    };

    template <typename ResourceDataType>
    class ResourceLoader : public ResourceLoaderParent {
        protected:
            ResourceLoader(std::initializer_list<std::string> types, std::initializer_list<std::string> extensions);

            virtual const ResourceDataType load(std::istream const &stream) const;
    };

    class ResourceManager {
        friend class ResourceLoaderParent;
        friend class ResourceParent;

        private:
            std::map<std::string, ResourcePrototype> discovered_resource_prototypes;
            std::map<std::string, ResourceParent*> loaded_resources;

            std::map<std::string, ResourceLoaderParent*> registered_loaders;
            std::map<std::string, std::string> extension_registrations;

            int unload_resource(std::string const &uid);

            template <typename ResourceDataType>
            void load_resource_trampoline(AsyncResourceRequestHandle &handle);

        public:
            void discover_resources(void);

            template <typename ResourceDataType>
            int register_loader(std::string const &type_id, ResourceLoader<ResourceDataType> const &loader);

            template <typename ResourceDataType>
            int get_resource(std::string const &uid, Resource<ResourceDataType> **const target);

            template <typename ResourceDataType>
            int try_get_resource(std::string const &uid, Resource<ResourceDataType> **const target) const;

            template <typename ResourceDataType>
            int load_resource(std::string const &uid);

            template <typename ResourceDataType>
            AsyncResourceRequestHandle load_reosurce_async(std::string const &uid,
                    const AsyncResourceRequestCallback callback);
    };

    class ResourceParent {
        friend class ResourceManager;

        private:
            ResourceManager &manager;

            std::atomic_bool ready;
            std::atomic<unsigned int> ref_count;

            std::vector<std::string> dependencies;

        public:
            const ResourcePrototype prototype;

            // god this is such a hack
            const struct {
                ResourceParent &parent;
                inline operator std::string() {
                    return parent.prototype.uid;
                }
            } uid {*this};
            const struct {
                ResourceParent &parent;
                inline operator std::string() {
                    return parent.prototype.type_id;
                }
            } type_id {*this};

            ResourceParent(ResourceParent &rhs):
                    manager(rhs.manager),
                    prototype(rhs.prototype),
                    ready(rhs.ready.load()),
                    ref_count(rhs.ref_count.load()) {
            }
            
            ResourceParent(ResourceParent &&rhs):
                    manager(rhs.manager),
                    prototype(std::move(rhs.prototype)),
                    ready(rhs.ready.load()),
                    ref_count(rhs.ref_count.load()) {
            }

            ResourceParent(ResourceManager &manager, const ResourcePrototype prototype):
                    manager(manager),
                    prototype(prototype),
                    ready(false),
                    ref_count(0) {
            }

            const bool is_ready(void) const {
                return ready;
            }

            void release(void) {
                if (--ref_count == 0) {
                    manager.unload_resource(prototype.uid);
                }
            }
    };

    template <typename ResourceDataType>
    class Resource : public ResourceParent {
        friend class ResourceManager;

        private:
            ResourceDataType data;

            Resource(ResourceManager &manager, const ResourcePrototype prototype):
                    ResourceParent(manager, prototype) {
            }

        public:
            ResourceDataType const &get_data(void) const {
                return data;
            }
    };

    ResourceManager &get_global_resource_manager(void);

}

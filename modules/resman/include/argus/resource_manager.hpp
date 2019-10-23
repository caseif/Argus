#pragma once

#include "argus/threading.hpp"

#include <atomic>
#include <fstream>
#include <future>
#include <istream>
#include <map>
#include <vector>

namespace argus {

    class Resource;
    class ResourceManager;

    struct ResourcePrototype {
        std::string uid;
        std::string type_id;
        std::string fs_path;
    };

    class ResourceException : public std::exception {
        private:
            const std::string msg;

        public:
            const std::string res_uid;

            ResourceException(std::string const &res_uid, const std::string msg):
                    res_uid(res_uid) {
            }

            const char *what(void) const noexcept override {
                return msg.c_str();
            }
    };

    class ResourceNotLoadedException : public ResourceException {
        public:
            ResourceNotLoadedException(std::string const &res_uid):
                    ResourceException(res_uid, "Resource is not loaded") {
            }
    };

    class ResourceLoadedException : public ResourceException {
        public:
            ResourceLoadedException(std::string const &res_uid):
                    ResourceException(res_uid, "Resource is already loaded") {
            }
    };

    class ResourceNotPresentException : public ResourceException {
        public:
            ResourceNotPresentException(std::string const &res_uid):
                    ResourceException(res_uid, "Resource does not exist") {
            }
    };

    class NoLoaderException : public ResourceException {
        public:
            const std::string resource_type;

            NoLoaderException(std::string const &res_uid, std::string const &type_id):
                    ResourceException(res_uid, "No registered loader for type"),
                    resource_type(type_id) {
            }
    };

    class LoadFailedException : public ResourceException {
        public:
            LoadFailedException(std::string const &res_uid):
                    ResourceException(res_uid, "Resource loading failed") {
            }
    };

    class ResourceLoader {
        friend class ResourceManager;

        private:
            const std::string type_id;
            const std::vector<std::string> extensions;
            
            std::vector<std::string> last_dependencies;

            virtual void *const load(std::istream &stream, const size_t size) const;

            virtual void unload(void *const data_ptr) const;

        protected:
            ResourceLoader(std::string type_id, std::initializer_list<std::string> extensions);

            int load_dependencies(std::initializer_list<std::string> dependencies);
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

        public:
            static ResourceManager &get_global_resource_manager(void);
    
            void discover_resources(void);

            int register_loader(std::string const &type_id, ResourceLoader *const loader);

            Resource &get_resource(std::string const &uid);

            Resource &try_get_resource(std::string const &uid) const;

            Resource &load_resource(std::string const &uid);

            std::future<Resource&> load_resource_async(std::string const &uid,
                    const std::function<void(Resource&)> callback);

            std::future<Resource&> load_resource_async(std::string const &uid);
    };

    class Resource {
        friend class ResourceManager;

        private:
            ResourceManager &manager;

            std::atomic<unsigned int> ref_count;

            std::vector<std::string> dependencies;

            void *const data_ptr;

            Resource(ResourceManager &manager, const ResourcePrototype prototype, void *const data,
                    std::vector<std::string> &dependencies);

            Resource operator=(Resource &ref) = delete;

        public:
            const ResourcePrototype prototype;

            // god this is such a hack
            const struct {
                Resource &parent;
                inline operator std::string(void) const {
                    return parent.prototype.uid;
                }
            } uid {*this};
            const struct {
                Resource &parent;
                inline operator std::string(void) const {
                    return parent.prototype.type_id;
                }
            } type_id {*this};

            Resource(Resource &rhs);
            
            Resource(Resource &&rhs);

            void release(void);

            template <typename DataType>
            DataType &get_data(void) {
                return *static_cast<DataType*>(data_ptr);
            }
    };

}

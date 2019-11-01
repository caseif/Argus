/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

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
                    res_uid(res_uid),
                    msg(msg) {
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
            /**
             * \brief The ResourceManager parent to this Resource.
             */
            ResourceManager &manager;

            /**
             * \brief The number of current handles to this Resource.
             *
             * \remark When the refcount reaches zero, the Resource will be
             *         unloaded.
             */
            std::atomic<unsigned int> ref_count;

            /**
             * \brief The UIDs of \link Resource Resources \endlink this one is
             *        dependent on.
             */
            std::vector<std::string> dependencies;

            /**
             * \brief A generic pointer to the data contained by this Resource.
             */
            void *const data_ptr;

            /**
             * \brief Constructs a new Resource.
             *
             * \param manager The parent ResourceManager of the new Resource.
             * \param prototype The \link ResourcePrototype prototype \endlink
             *        of the new Resource.
             * \param data A pointer to the resource data.
             * \param dependencies The UIDs of Resources the new one is
             *        dependent on.
             */
            Resource(ResourceManager &manager, const ResourcePrototype prototype, void *const data,
                    std::vector<std::string> &dependencies);

            Resource(Resource &res) = delete;

            Resource operator=(Resource &ref) = delete;

        public:
            const ResourcePrototype prototype;

            // the uid and type_id fields are inline structs which implement
            // a std::string conversion operator so as to allow the fields of
            // the same name from the underlying prototype to be used in a
            // mostly-transparent manner

            /**
             * \brief The UID of this resource.
             *
             * \attention This is a proxy to the same field of the underlying
             *            ResourcePrototype and has been implemented in a way
             *            to allow a direct proxy while maintaining field
             *            syntax.
             */
            const struct {
                Resource &parent;
                /**
                 * \brief Extracts the resource's UID from its
                 *        ResourcePrototype.
                 *
                 * \return The resource's UID.
                 */
                inline operator std::string(void) const {
                    return parent.prototype.uid;
                }
            } uid {*this};
            /**
             * \brief The type ID of this resource.
             *
             * \attention This is a proxy to the same field of the underlying
             *            ResourcePrototype and has been implemented in a way
             *            to allow a direct proxy while maintaining field
             *            syntax.
             */
            const struct {
                Resource &parent;
                /**
                 * \brief Extracts the resource's type ID from its
                 *        ResourcePrototype.
                 *
                 * \return The resource's type ID.
                 */
                inline operator std::string(void) const {
                    return parent.prototype.type_id;
                }
            } type_id {*this};

            Resource(Resource &&rhs);

            /**
             * \brief Releases a handle on this Resource.
             *
             * \remark This simply decrements an internal refcount, as the class
             *         has no way of tracking specific acquisitions.
             */
            void release(void);

            /**
             * \brief Gets the underlying data of this Resource.
             *
             * \tparam DataType The type of data contained by this Resource.
             *
             * \return The Resource data.
             */
            template <typename DataType>
            DataType &get_data(void) {
                return *static_cast<DataType*>(data_ptr);
            }
    };

}

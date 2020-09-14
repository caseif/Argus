/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

/**
 * \file argus/resource_manager.hpp
 *
 * High-level resource management API.
 */

#pragma once

#include <atomic>
#include <fstream>
#include <future>
#include <istream>
#include <map>
#include <vector>

namespace argus {

    class Resource;
    class ResourceManager;

    /**
     * \brief The minimum information required to uniquely identify and locate
     *        a resource.
     */
    struct ResourcePrototype {
        /**
         * \brief The unique identifier of the resource.
         *
         * The UID does not include a file extension, and the path separator for
         * resources is a dot (.). For instance, a loose resource file with the
         * relative path `foo/bar/resource.dat` can be accessed with UID
         * `foo.bar.resource`.
         */
        std::string uid;
        /**
         * \brief The ID of the resource's type.
         */
        std::string type_id;
        /**
         * \brief The path to the resource on the filesystem.
         *
         * \attention This will point either to the loose resource file on the
         *            disk, or the archive containing the resource data.
         */
        std::string fs_path;
        /**
         * \brief Whether the resource is in an archive.
         *
         * If `false`, the resource is present as a loose file on the disk.
         */
        bool archived;
    };

    /**
     * \brief Represents an exception related to a Resource.
     */
    class ResourceException : public std::exception {
        private:
            const std::string msg;

        public:
            /**
             * \brief The UID of the Resource asssociated with this exception.
             */
            const std::string res_uid;

            /**
             * \brief Constructs a ResourceException.
             *
             * \param res_uid The UID of the Resource associated with the
             *        exception.
             * \param msg The message associated with the exception.
             */
            ResourceException(const std::string &res_uid, const std::string msg):
                    res_uid(res_uid),
                    msg(msg) {
            }

            /**
             * \copydoc std::exception::what()
             *
             * \return The exception message.
             */
            const char *what(void) const noexcept override {
                return msg.c_str();
            }
    };

    /**
     * \brief Thrown when a Resource not in memory is accessed without being
     *        loaded first.
     */
    class ResourceNotLoadedException : public ResourceException {
        public:
            /**
             * \brief Constructs a new exception.
             *
             * \param res_uid The UID of the Resource associated with the
             *        exception.
             */
            ResourceNotLoadedException(const std::string &res_uid):
                    ResourceException(res_uid, "Resource is not loaded") {
            }
    };

    /**
     * \brief Thrown when a load is requested for an already-loaded Resource.
     */
    class ResourceLoadedException : public ResourceException {
        public:
            /**
             * \brief Constructs a new exception.
             *
             * \param res_uid The UID of the Resource associated with the
             *        exception.
             */
            ResourceLoadedException(const std::string &res_uid):
                    ResourceException(res_uid, "Resource is already loaded") {
            }
    };

    /**
     * \brief Thrown when a Resource not in memory is requested without being
     *        loaded first.
     */
    class ResourceNotPresentException : public ResourceException {
        public:
            /**
             * \brief Constructs a new exception.
             *
             * \param res_uid The UID of the Resource associated with the
             *        exception.
             */
            ResourceNotPresentException(const std::string &res_uid):
                    ResourceException(res_uid, "Resource does not exist") {
            }
    };

    /**
     * \brief Thrown when a load is requested for a Resource with a type which
     *        is missing a registered loader.
     */
    class NoLoaderException : public ResourceException {
        public:
            /**
             * \brief The type of Resource for which a load failed.
             */
            const std::string resource_type;

            /**
             * \brief Constructs a new exception.
             *
             * \param res_uid The UID of the Resource associated with the
             *        exception.
             * \param type_id The type of Resource for which a load failed.
             */
            NoLoaderException(const std::string &res_uid, const std::string &type_id):
                    ResourceException(res_uid, "No registered loader for type"),
                    resource_type(type_id) {
            }
    };

    /**
     * \brief Thrown when a load is requested for a Resource present on disk,
     *        but said load fails for any reason.
     */
    class LoadFailedException : public ResourceException {
        public:
            /**
             * \brief Constructs a new exception.
             *
             * \param res_uid The UID of the Resource associated with the
             *        exception.
             */
            LoadFailedException(const std::string &res_uid):
                    ResourceException(res_uid, "Resource loading failed") {
            }
    };

    /**
     * \brief Handles deserialization of Resource data.
     */
    class ResourceLoader {
        friend class ResourceManager;

        private:
            /**
             * \brief The ID of the type handled by this loader.
             */
            const std::string type_id;
            /**
             * \brief The file extensions this loader can handle.
             */
            const std::vector<std::string> extensions;

            /**
             * \brief The dependencies of the Resource last loaded.
             */
            std::vector<std::string> last_dependencies;

            /**
             * \brief Loads a resource from an std::istream.
             *
             * \param stream THe stream to load the Resource from.
             * \param size The size in bytes of the resource data.
             */
            virtual void *const load(std::istream &stream, const size_t size) const;

            /**
             * \brief Performs necessary deinitialization for loaded resource
             *        data.
             * \param data_ptr A pointer to the resource data to be deinitialized.
             */
            virtual void unload(void *const data_ptr) const;

        protected:
            /**
             * \brief Constructs a new ResourceLoader.
             *
             * \param type_id The ID of the type handled by this loader.
             * \param extensions The file extensions handled by this loader.
             */
            ResourceLoader(std::string type_id, std::initializer_list<std::string> extensions);

            /**
             * \brief Loads \link Resource Resources \endlink this one is
             *        dependent on.
             *
             * Subclasses should invoke this during Resource loading.
             *
             * \param dependencies A std::vector of UIDs of dependency
             *        \link Resource Resources \endlink.
             *
             * \throw ResourceException If any dependency Resource cannot be
             *        loaded.
             */
            void load_dependencies(std::initializer_list<std::string> dependencies);
    };

    /**
     * \brief Manages Resource lifetimes and provides a high-level interface for
     *        loading, retrieving, and unloading them.
     */
    class ResourceManager {
        friend class ResourceLoader;
        friend class Resource;

        private:
            /**
             * \brief Prototypes for all resources discovered on the filesystem.
             */
            std::map<std::string, ResourcePrototype> discovered_resource_prototypes;
            /**
             * \brief All currently loaded resources.
             */
            std::map<std::string, Resource*> loaded_resources;

            /**
             * \brief All currently registered resource loaders.
             */
            std::map<std::string, ResourceLoader*> registered_loaders;
            /**
             * \brief All current extension registrations, with extensions being
             *        mapped to formal types.
             */
            std::map<std::string, std::string> extension_registrations;

            /**
             * \brief Unloads the Resource with the given UID.
             *
             * \throw ResourceNotLoadedException If the Resource is not
             *        currently loaded.
             */
            int unload_resource(const std::string &uid);

        public:
            /**
             * \brief Gets the global ResourceManager instance.
             *
             * \return The global ResourceManager.
             */
            static ResourceManager &get_global_resource_manager(void);

            /**
             * \brief Discovers all present resources from the filesystem.
             */
            void discover_resources(void);

            /**
             * \brief Registers a ResourceLoader for the given type.
             *
             * \param type_id The resource type ID to register a loader for.
             * \param loader The ResourceLoader to register.
             *
             * \throw std::invalid_argument If a loader is already registered
             *        for the provided type.
             */
            void register_loader(const std::string &type_id, ResourceLoader *const loader);

            /**
             * \brief Attempts to get the Resource with the given UID.
             *
             * \param uid The UID of the Resource to retrieve.
             *
             * \return The retrieved Resource.
             *
             * \throw ResourceNotPresentException If no Resource with the given
             *        UID exists.
             *
             * \attention This method will attempt to load the Resource if it is
             *            not already in memory.
             *
             * \sa ResourceManager#get_resource_async
             */
            Resource &get_resource(const std::string &uid);

            /**
             * \brief Attempts to get the Resource with the given UID, failing
             *        if it is not already loaded.
             *
             * \param uid The UID of the Resource to retrieve.
             *
             * \return The retrieved Resource.
             *
             * \throw ResourceNotPresentException If no Resource with the given
             *        UID exists.
             * \throw ResourceNotLoadedException If the Resource is not already
             *        loaded.
             */
            Resource &try_get_resource(const std::string &uid) const;

            /**
             * \brief Attempts to load the Resource with the given UID, failing
             *        if it is already loaded.
             *
             * This method differs semantically from
             * ResourceManager#get_resource in that it expects the Resource to
             * not yet be loaded.
             *
             * \param uid The UID of the Resource to load.
             *
             * \return The loaded Resource.
             *
             * \sa ResourceManager#load_resource_async
             */
            Resource &load_resource(const std::string &uid);

            /**
             * \brief Attempts to retrieve the Resource with the given UID
             *        asynchronously, loading it if it is not already loaded.
             *
             * \param uid The UID of the Resource to retrieve.
             * \param callback A callback to execute upon completion of the load
             *        routine. This callback _must_ be thread-safe.
             *
             * \return A std::future which will provide the retrieved Resource
             *         (or any exception thrown by the load routine).
             *
             * \sa ResourceManager#get_resource
             */
            std::future<Resource&> get_resource_async(const std::string &uid,
                    const std::function<void(Resource&)> callback);
            /**
             * \brief Attempts to retrieve the Resource with the given UID
             *        asynchronously, loading it if it is not already loaded.
             *
             * \param uid The UID of the Resource to retrieve.
             *
             * \return A std::future which will provide the retrieved Resource
             *         (or any exception thrown by the load routine).
             *
             * \sa ResourceManager#get_resource
             */
            std::future<Resource&> get_resource_async(const std::string &uid);

            /**
             * \brief Attempts to load the Resource with the given UID
             *        asynchronously, failing if it is already loaded.
             *
             * \param uid The UID of the Resource to load.
             * \param callback A callback to execute upon completion of the load
             *        routine. This callback _must_ be thread-safe.
             *
             * \return A std::future which will provide the loaded Resource (or
             *         any exception thrown by the load routine).
             *
             * \sa ResourceManager#load_resource
             */
            std::future<Resource&> load_resource_async(const std::string &uid,
                    const std::function<void(Resource&)> callback);
            /**
             * \brief Attempts to load the Resource with the given UID
             *        asynchronously, failing if it is already loaded.
             *
             * \param uid The UID to look up.
             *
             * \return A std::future which will provide the loaded Resource (or
             *         any exception thrown by the load routine).
             *
             * \sa ResourceManager#load_resource
             */
            std::future<Resource&> load_resource_async(const std::string &uid);
    };

    /**
     * \brief Represents semantically structured data loaded from the
     *        filesystem.
     */
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
            /**
             * \brief The prototype of this Resource.
             */
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
                /**
                 * \brief The parent Resource to proxy for.
                 */
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
                /**
                 * \brief The parent Resource to proxy to.
                 */
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

            /**
             * \brief The move constructor.
             *
             * \param rhs The Resource to move.
             */
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

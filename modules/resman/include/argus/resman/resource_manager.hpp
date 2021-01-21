#pragma once

#include <functional>
#include <future>
#include <map>
#include <string>

namespace argus {
    // forward declarations
    class Resource;
    class ResourceLoader;
    struct pimpl_ResourceManager;

    /**
     * \brief Manages Resource lifetimes and provides a high-level interface for
     *        loading, retrieving, and unloading them.
     */
    class ResourceManager {
        public:
            pimpl_ResourceManager *pimpl;

            /**
             * \brief Gets the global ResourceManager instance.
             *
             * \return The global ResourceManager.
             */
            static ResourceManager &get_global_resource_manager(void);

            /**
             * \brief Constructs a new ResourceManager.
             */
            ResourceManager(void);

            ResourceManager(ResourceManager&) = delete;

            ResourceManager(ResourceManager&&) = delete;

            /**
             * \brief Destroys the ResourceManager.
             */
            ~ResourceManager(void);

            /**
             * \brief Unloads the Resource with the given UID.
             *
             * \throw ResourceNotLoadedException If the Resource is not
             *        currently loaded.
             */
            int unload_resource(const std::string &uid);

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
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
        friend class Resource;

        private:
            /**
             * \brief Unloads the Resource with the given UID.
             *
             * \throw ResourceNotLoadedException If the Resource is not
             *        currently loaded.
             */
            int unload_resource(const std::string &uid);

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
        public:
            pimpl_ResourceManager *pimpl;

            /**
             * \brief Gets the global ResourceManager instance.
             *
             * \return The global ResourceManager.
             */
            static ResourceManager &instance(void);

            /**
             * \brief Discovers all present resources from the filesystem.
             */
            void discover_resources(void);

            /**
             * \brief Loads an in-memory ARP package for this resource manager.
             *
             * \param buf The buffer containing the in-memory package.
             * \param len The length of the in-memory package.
             */
            void add_memory_package(const unsigned char *buf, size_t len);

            /**
             * \brief Registers a ResourceLoader for the given type.
             *
             * \param loader The ResourceLoader to register.
             *
             * \throw std::invalid_argument If a loader is already registered
             *        for the provided type.
             */
            void register_loader(ResourceLoader &loader);

            /**
             * \brief Registers extension mappings for this manager, overriding
             *        any conflicting presets.
             *
             * \param mappings The mappings to register, with keys being
             *        extensions and values being media types.
             */
            void register_extension_mappings(const std::map<std::string, std::string> &mappings);

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
             * \sa ResourceManager#try_get_resource
             */
            Resource &get_resource(const std::string &uid);

            /**
             * \brief Attempts to get the Resource with the given UID without
             *        incrementing the Resource's refcount. This function
             *        assumes the Resource is already loaded and will fail if it
             *        is not.
             *
             * \param uid The UID of the Resource to retrieve.
             *
             * \return The retrieved Resource.
             *
             * \throw ResourceNotPresentException If no Resource with the given
             *        UID exists.
             * \throw ResourceNotLoadedException If the Resource is not already
             *        loaded.
             *
             * \warning This should not be used unless you know what you are
             *          doing. This function is intended for use with dependent
             *          Resources guaranteed to have a lifetime extending until
             *          or beyond that of the returned reference, and improper
             *          use may lead to incorrect or strange behavior.
             *
             * \sa ResourceManager#get_resource
             */
            Resource &get_resource_weak(const std::string &uid);

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
             *
             * \sa ResourceManager#get_resource
             */
            Resource &try_get_resource(const std::string &uid) const;

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
                    const std::function<void(Resource&)> &callback = nullptr);

            /**
             * \brief Creates a Resource with the given UID from data presently
             *        in memory.
             *
             * \param uid The UID of the new Resource.
             * \param media_type The media type of the Resource.
             * \param data The in-memory data of the Resource.
             * \param len The size of the Resource in bytes.
             *
             * \return The created Resource.
             *
             * \throw ResourceLoadedException If a Resource with the given UID
             *        is already loaded.
             */
            Resource &create_resource(const std::string &uid, const std::string &media_type, const void *data,
                    size_t len);
    };
}

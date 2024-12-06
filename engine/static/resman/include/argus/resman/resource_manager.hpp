/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/result.hpp"

#include <functional>
#include <future>
#include <map>
#include <string>
#include <typeindex>

namespace argus {
    // forward declarations
    class Resource;

    class ResourceLoader;

    struct pimpl_ResourceManager;

    enum class ResourceErrorReason {
        Generic,
        NotFound,
        NotLoaded,
        AlreadyLoaded,
        NoLoader,
        LoadFailed,
        MalformedContent,
        InvalidContent,
        UnsupportedContent,
        UnexpectedReferenceType,
    };

    struct ResourceError {
        ResourceErrorReason reason;
        std::string uid;
        std::string info;

        std::string to_string(void) const;
    };

    /**
     * @brief Manages Resource lifetimes and provides a high-level interface for
     *        loading, retrieving, and unloading them.
     */
    class ResourceManager {
        friend class Resource;

      private:
        /**
         * @brief Unloads the Resource with the given UID.
         */
        [[nodiscard]] Result<void, ResourceError> unload_resource(const std::string &uid);

        /**
         * @brief Constructs a new ResourceManager.
         */
        ResourceManager(void);

        /**
         * @brief Destroys the ResourceManager.
         */
        ~ResourceManager(void);

        [[nodiscard]] Result<Resource &, ResourceError> create_resource(const std::string &uid,
                const std::string &media_type, void *obj, std::type_index type);

      public:
        /**
         * @brief Gets the global ResourceManager instance.
         *
         * @return The global ResourceManager.
         */
        static ResourceManager &instance(void);

        pimpl_ResourceManager *m_pimpl;

        ResourceManager(ResourceManager &) = delete;

        ResourceManager(ResourceManager &&) = delete;

        /**
         * @brief Discovers all present resources from the filesystem.
         */
        void discover_resources(void);

        /**
         * @brief Loads an in-memory ARP package for this resource manager.
         *
         * @param buf The buffer containing the in-memory package.
         * @param len The length of the in-memory package.
         */
        void add_memory_package(const unsigned char *buf, size_t len);

        /**
         * @brief Registers a ResourceLoader for the given type.
         *
         * @param loader The ResourceLoader to register.
         */
        void register_loader(ResourceLoader &loader);

        /**
         * @brief Registers extension mappings for this manager, overriding
         *        any conflicting presets.
         *
         * @param mappings The mappings to register, with keys being
         *        extensions and values being media types.
         */
        void register_extension_mappings(const std::map<std::string, std::string> &mappings);

        /**
         * @brief Attempts to get the Resource with the given UID.
         *
         * @param uid The UID of the Resource to retrieve.
         *
         * @return The retrieved Resource.
         *
         * @attention This method will attempt to load the Resource if it is
         *            not already in memory.
         *
         * @sa ResourceManager#get_resource_async
         * @sa ResourceManager#try_get_resource
         */
        [[nodiscard]] Result<Resource &, ResourceError> get_resource(const std::string &uid);

        /**
         * @brief Attempts to get the Resource with the given UID without
         *        incrementing the Resource's refcount. This function
         *        assumes the Resource is already loaded and will fail if it
         *        is not.
         *
         * @param uid The UID of the Resource to retrieve.
         *
         * @return The retrieved Resource.
         *
         * @warning This should not be used unless you know what you are
         *          doing. This function is intended for use with dependent
         *          Resources guaranteed to have a lifetime extending until
         *          or beyond that of the returned reference, and improper
         *          use may lead to incorrect or strange behavior.
         *
         * @sa ResourceManager#get_resource
         */
        [[nodiscard]] Result<Resource &, ResourceError> get_resource_weak(const std::string &uid);

        /**
         * @brief Attempts to get the Resource with the given UID, failing
         *        if it is not already loaded.
         *
         * @param uid The UID of the Resource to retrieve.
         *
         * @return The retrieved Resource.
         *
         * @sa ResourceManager#get_resource
         */
        [[nodiscard]] Result<Resource &, ResourceError> try_get_resource(const std::string &uid) const;

        /**
         * @brief Attempts to retrieve the Resource with the given UID
         *        asynchronously, loading it if it is not already loaded.
         *
         * @param uid The UID of the Resource to retrieve.
         * @param callback A callback to execute upon completion of the load
         *        routine. This callback _must_ be thread-safe.
         *
         * @return A std::future which will provide the retrieved Resource
         *         (or any error returned by the load routine).
         *
         * @sa ResourceManager#get_resource
         */
        std::future<Result<Resource &, ResourceError>> get_resource_async(const std::string &uid,
                const std::function<void(Result<Resource &, ResourceError>)> &callback = nullptr);

        /**
         * @brief Creates a Resource with the given UID from data presently
         *        in memory without validating the source object type.
         *
         * @param uid The UID of the new Resource.
         * @param media_type The media type of the Resource.
         * @param handle A handle to the object in memory to be copied.
         *
         * @return The created Resource.
         */
        [[nodiscard]] Result<Resource &, ResourceError> create_resource_unchecked(const std::string &uid,
                const std::string &media_type, const void *handle);

        template<typename T>
        [[nodiscard]] Result<Resource &, ResourceError> create_resource(const std::string &uid,
                const std::string &media_type, T &&res_obj) {
            return create_resource(uid, media_type, &res_obj, std::type_index(typeid(res_obj)));
        }
    };
}

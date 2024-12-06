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

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include <initializer_list>
#include <istream>
#include <map>
#include <string>
#include <typeindex>
#include <vector>

#include <cstddef>

namespace argus {
    // forward declarations
    struct ResourceError;
    class ResourceManager;

    struct pimpl_ResourceLoader;

    /**
     * @brief Handles deserialization of Resource data.
     */
    class ResourceLoader {
        friend class ResourceManager;

      private:
        pimpl_ResourceLoader *m_pimpl;

        /**
         * @brief Loads a resource from an std::istream.
         *
         * @param proto The prototype of the Resource being loaded.
         * @param stream The stream to load the Resource from.
         * @param size The size in bytes of the Resource data.
         */
        virtual Result<void *, ResourceError> load(ResourceManager &manager, const ResourcePrototype &proto,
                std::istream &stream, size_t size) = 0;

        virtual Result<void *, ResourceError> copy(ResourceManager &manager, const ResourcePrototype &proto,
                const void *src, std::optional<std::type_index> type) = 0;

        /**
         * @brief Performs necessary deinitialization for loaded resource
         *        data.
         * @param data_ptr A pointer to the resource data to be deinitialized.
         */
        virtual void unload(void *data_ptr) = 0;

      protected:
        static Result<void *, ResourceError> make_ok_result(void *ptr);

        static Result<void *, ResourceError> make_err_result(ResourceErrorReason reason, const ResourcePrototype &proto,
                std::string msg = "");

        /**
         * @brief Constructs a new ResourceLoader.
         *
         * @param media_types The media types handled by this loader.
         */
        ResourceLoader(std::initializer_list<std::string> media_types);

        /**
         * @brief Constructs a new ResourceLoader.
         *
         * @param media_types The media types handled by this loader.
         */
        ResourceLoader(std::vector<std::string> media_types);

        /**
         * @brief Destroys the ResourceLoader.
         */
        virtual ~ResourceLoader(void);

        /**
         * @brief Loads \link Resource Resources \endlink this one is
         *        dependent on.
         *
         * Subclasses should invoke this during Resource loading.
         *
         * @param manager The ResourceManager requesting the load.
         * @param dependencies A std::vector of UIDs of dependency
         *        \link Resource Resources \endlink.
         */
        [[nodiscard]] Result<std::map<std::string, const Resource *>, ResourceError> load_dependencies(
                ResourceManager &manager, const std::vector<std::string> &dependencies);
    };
}

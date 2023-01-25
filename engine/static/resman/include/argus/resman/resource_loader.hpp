/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/resman/resource.hpp"

#include <initializer_list>
#include <istream>
#include <map>
#include <string>
#include <typeindex>
#include <vector>

#include <cstddef>

namespace argus {
    // forward declarations
    class ResourceManager;

    struct pimpl_ResourceLoader;

    /**
     * \brief Handles deserialization of Resource data.
     */
    class ResourceLoader {
        friend class ResourceManager;

      private:
        pimpl_ResourceLoader *pimpl;

        /**
         * \brief Loads a resource from an std::istream.
         *
         * \param proto The prototype of the Resource being loaded.
         * \param stream The stream to load the Resource from.
         * \param size The size in bytes of the Resource data.
         */
        virtual void *load(ResourceManager &manager, const ResourcePrototype &proto,
                std::istream &stream, size_t size) const = 0;

        virtual void *copy(ResourceManager &manager, const ResourcePrototype &proto,
                void *src, std::type_index type) const = 0;

        /**
         * \brief Performs necessary deinitialization for loaded resource
         *        data.
         * \param data_ptr A pointer to the resource data to be deinitialized.
         */
        virtual void unload(void *data_ptr) const = 0;

      protected:
        /**
         * \brief Constructs a new ResourceLoader.
         *
         * \param media_types The media types handled by this loader.
         */
        ResourceLoader(std::initializer_list<std::string> media_types);

        /**
         * \brief Destroys the ResourceLoader.
         */
        virtual ~ResourceLoader(void);

        /**
         * \brief Loads \link Resource Resources \endlink this one is
         *        dependent on.
         *
         * Subclasses should invoke this during Resource loading.
         *
         * \param manager The ResourceManager requesting the load.
         * \param dependencies A std::vector of UIDs of dependency
         *        \link Resource Resources \endlink.
         *
         * \throw ResourceException If any dependency Resource cannot be
         *        loaded.
         */
        std::map<std::string, const Resource *> load_dependencies(ResourceManager &manager,
                const std::vector<std::string> &dependencies) const;
    };
}

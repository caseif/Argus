/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module resman
#include "argus/resman/resource.hpp"

#include <initializer_list>
#include <istream>
#include <string>
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
            virtual void *const load(const ResourcePrototype &proto, std::istream &stream, const size_t size) const;

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
             * \brief Destroys the ResourceLoader.
             */
            ~ResourceLoader(void);

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
            void load_dependencies(ResourceManager &manager, std::initializer_list<std::string> dependencies);
    };
}

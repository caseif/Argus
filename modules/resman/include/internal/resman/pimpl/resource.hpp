/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "arp/arp.h"

#include <atomic>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Resource;
    class ResourceLoader;
    class ResourceManager;

    struct pimpl_Resource {
        friend class ResourceManager;

        /**
         * \brief The ResourceManager parent to this Resource.
         */
        ResourceManager &manager;

        /**
         * \brief The ResourceLoader responsible for the handling of the loading
         *        and unloading of the Resource.
         */
        const ResourceLoader &loader;

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
         * \brief The ARP resource backing this Resource.
         */
        arp_resource_t *arp_resource;

        pimpl_Resource(ResourceManager &manager, const ResourceLoader &loader, void *const data_ptr,
                std::vector<std::string> &dependencies, unsigned int ref_count = 1):
            manager(manager),
            loader(loader),
            data_ptr(data_ptr),
            dependencies(dependencies),
            ref_count(ref_count) {
        }

        pimpl_Resource(pimpl_Resource&) = delete;

        pimpl_Resource(pimpl_Resource&&) = delete;
    };
}

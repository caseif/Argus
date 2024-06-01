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
        arp_resource_t *arp_resource{nullptr};

        pimpl_Resource(ResourceManager &manager, const ResourceLoader &loader, void *const data_ptr,
                const std::vector<std::string> &dependencies, unsigned int ref_count = 1) :
                manager(manager),
                loader(loader),
                ref_count(ref_count),
                dependencies(dependencies),
                data_ptr(data_ptr) {
        }

        pimpl_Resource(pimpl_Resource &) = delete;

        pimpl_Resource(pimpl_Resource &&) = delete;
    };
}

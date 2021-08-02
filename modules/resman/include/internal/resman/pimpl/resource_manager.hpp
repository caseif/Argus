/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"

#include "arp/unpack/set.h"

#include <map>
#include <string>

namespace argus {
    struct pimpl_ResourceManager {
        /**
         * \brief Prototypes for all resources discovered on the filesystem.
         */
        std::map<std::string, ResourcePrototype> discovered_fs_protos;

        /**
         * \brief A set of all ARP packages currently loaded.
         */
        ArpPackageSet package_set{nullptr};

        /**
         * \brief All currently loaded resources.
         */
        std::map<std::string, Resource*> loaded_resources;

        /**
         * \brief All currently registered resource loaders.
         */
        std::map<const std::string, ResourceLoader*> registered_loaders;

        /**
         * \brief All current extension registrations, with extensions being
         *        mapped to formal types.
         */
        std::map<std::string, std::string> extension_mappings;

        pimpl_ResourceManager(void) = default;

        pimpl_ResourceManager(const pimpl_ResourceManager&) = delete;

        pimpl_ResourceManager(pimpl_ResourceManager&&) = delete;
    };
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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
         * \brief Whether discovery of resources from the filesystem has taken
         *        place.
         */
        bool discovery_done;

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

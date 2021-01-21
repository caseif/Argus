#pragma once

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"

#include <map>
#include <string>

namespace argus {
    struct pimpl_ResourceManager {
        /**
         * \brief Prototypes for all resources discovered on the filesystem.
         */
        std::map<std::string, ResourcePrototype> discovered_resource_prototypes;
        /**
         * \brief All currently loaded resources.
         */
        std::map<std::string, Resource*> loaded_resources;

        /**
         * \brief All currently registered resource loaders.
         */
        std::map<std::string, ResourceLoader*> registered_loaders;
        /**
         * \brief All current extension registrations, with extensions being
         *        mapped to formal types.
         */
        std::map<std::string, std::string> extension_registrations;

        pimpl_ResourceManager(void) {
        }

        pimpl_ResourceManager(const pimpl_ResourceManager&) = delete;

        pimpl_ResourceManager(pimpl_ResourceManager&&) = delete;
    };
}
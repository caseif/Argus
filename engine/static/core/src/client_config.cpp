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

#include "argus/lowlevel/filesystem.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/streams.hpp"
#include "internal/lowlevel/logging.hpp"

#include "argus/core/client_config.hpp"

#include "arp/arp.h"
#include "nlohmann/json.hpp"

#include <sstream>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <unistd.h>
#endif

#define EXT_SEPARATOR "."

#define ARP_NS_SEPARATOR ":"

#define CONFIG_BASE_NAME "client"
#define CONFIG_FILE_EXT "json"
#define CONFIG_FILE_NAME CONFIG_BASE_NAME EXT_SEPARATOR CONFIG_FILE_EXT
#define CONFIG_MEDIA_TYPE "application/json"

#define RESOURCES_DIR "resources"
#define ARP_EXT "arp"

namespace argus {
    static inline bool _ends_with(const std::string &haystack, const std::string &needle)
    {
        if (needle.size() > haystack.size()) {
            return false;
        }
        return std::equal(needle.rbegin(), needle.rend(), haystack.rbegin());
    }

    static std::string _get_resources_path(void) {
        auto cwd = getcwd(NULL, 0);
        std::string res_dir = std::string(cwd) + PATH_SEPARATOR + RESOURCES_DIR;
        free(cwd);
        return res_dir;
    }

    static bool _ingest_config_content(std::istream &stream) {
        UNUSED(stream);
        //TODO
        return true;
    }

    static bool _ingest_config_from_file(const std::string &ns) {
        std::string config_path = _get_resources_path()
            + PATH_SEPARATOR + ns
            + PATH_SEPARATOR + CONFIG_FILE_NAME;

        if (!is_regfile(config_path)) {
            return false;
        }

        auto handle = FileHandle::create(config_path, FILE_MODE_READ);

        std::ostringstream str_stream;
        std::ifstream stream;
        handle.to_istream(0, stream);

        if (!_ingest_config_content(stream)) {
            _ARGUS_FATAL("Failed to parse config from file at %s", config_path.c_str());
        }

        handle.release();

        _ARGUS_INFO("Successfully loaded config from file at %s", config_path.c_str());
        return true;
    }

    static bool _ingest_config_from_arp(const std::string &ns) {
        auto res_dir = _get_resources_path();

        std::vector<std::string> children;
        try {
            children = list_directory_entries(res_dir);
        } catch (std::exception &ex) {
            _ARGUS_WARN("Failed to search for config in ARP files: %s", ex.what());
            return false;
        }

        std::vector<std::string> candidate_packages;

        for (const std::string &child : children) {
            if (!_ends_with(child, EXT_SEPARATOR ARP_EXT)) {
                continue;
            }
            
            auto full_child_path = res_dir + PATH_SEPARATOR + child;

            if (!arp_is_base_archive(full_child_path.c_str())) {
                continue;
            }

            arp_package_meta_t meta;
            auto rc = arp_load_from_file(full_child_path.c_str(), &meta, nullptr);
            if (rc != 0) {
                _ARGUS_WARN("Failed to load package %s while searching for config (libarp says: %s)",
                    child.c_str(), arp_get_error());
                continue;
            }

            if (strcmp(meta.package_namespace, ns.c_str()) == 0) {
                candidate_packages.push_back(child);
            }
        }

        for (const std::string &candidate_name : candidate_packages) {
            _ARGUS_DEBUG("Searching for client config in package %s (namespace matches)", candidate_name.c_str());

            auto full_path = res_dir + PATH_SEPARATOR + candidate_name;

            ArpPackage pack;
            auto rc = arp_load_from_file(full_path.c_str(), nullptr, &pack);
            if (rc != 0) {
                _ARGUS_WARN("Failed to load package at %s while searching for config (libarp says: %s)",
                    candidate_name.c_str(), arp_get_error());
                continue;
            }

            arp_resource_meta_t res_meta;
            rc = arp_find_resource(pack, (ns + ARP_NS_SEPARATOR + CONFIG_BASE_NAME).c_str(), &res_meta);
            if (rc == E_ARP_RESOURCE_NOT_FOUND) {
                _ARGUS_DEBUG("Did not find config in package %s", candidate_name.c_str());
                continue;
            } else if (rc != 0) {
                _ARGUS_WARN("Failed to find config in package %s (libarp says: %s)",
                    candidate_name.c_str(), arp_get_error());
                continue;
            }

            if (strcmp(res_meta.media_type, CONFIG_MEDIA_TYPE) != 0) {
                _ARGUS_WARN("File \"%s\" in package %s has unexpected media type %s, cannot load as client config",
                    CONFIG_BASE_NAME, candidate_name.c_str(), res_meta.media_type);
                continue;
            }

            assert(rc == 0);

            auto *res = arp_load_resource(&res_meta);
            if (res == nullptr) {
                _ARGUS_WARN("Failed to load config from package %s (libarp says: %s)",
                    candidate_name.c_str(), arp_get_error());
                continue;
            }

            IMemStream stream(res->data, res->meta.size);
            _ingest_config_content(stream);

            _ARGUS_INFO("Successfully loaded config from package at %s", candidate_name.c_str());
            return true;
        }

        return false;
    }

    void load_client_config(const std::string &config_namespace) {
        std::string cur_path;
        if (!_ingest_config_from_file(config_namespace)
            && !_ingest_config_from_arp(config_namespace)) {
            _ARGUS_FATAL("Failed to locate " CONFIG_FILE_NAME " in namespace %s", config_namespace.c_str());
        }
    }
}

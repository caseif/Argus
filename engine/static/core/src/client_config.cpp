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
#include "argus/core/downstream_config.hpp"
#include "internal/core/client_properties.hpp"
#include "internal/core/engine_config.hpp"

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

#define KEY_CLIENT "client"
#define KEY_CLIENT_ID "id"
#define KEY_CLIENT_NAME "name"
#define KEY_CLIENT_VERSION "version"

#define KEY_ENGINE "engine"
#define KEY_ENGINE_MODULES "modules"
#define KEY_ENGINE_RENDER_BACKENDS "render_backends"
#define KEY_ENGINE_TICKRATE "target_tickrate"
#define KEY_ENGINE_FRAMERATE "target_framerate"

#define KEY_WINDOW "window"
#define KEY_WINDOW_ID "id"
#define KEY_WINDOW_TITLE "title"
#define KEY_WINDOW_MODE "mode"
#define KEY_WINDOW_VSYNC "vsync"
#define KEY_WINDOW_MOUSE "mouse"
#define KEY_WINDOW_MOUSE_VISIBLE "visible"
#define KEY_WINDOW_MOUSE_CAPTURE "capture"
#define KEY_WINDOW_POSITION "position"
#define KEY_WINDOW_POSITION_X "x"
#define KEY_WINDOW_POSITION_Y "y"
#define KEY_WINDOW_DIMENSIONS "dimensions"
#define KEY_WINDOW_DIMENSIONS_W "width"
#define KEY_WINDOW_DIMENSIONS_H "height"

#define KEY_BINDINGS "bindings"
#define KEY_BINDINGS_DEFAULT_BINDINGS_RESOURCE "default_bindings_resource"
#define KEY_BINDINGS_SAVE_USER_BINDINGS "save_user_bindings"

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

    static void _ingest_client_properties(nlohmann::json client_obj) {
        if (client_obj.contains(KEY_CLIENT_ID) && client_obj[KEY_CLIENT_ID].is_string()) {
            get_client_properties().id = client_obj[KEY_CLIENT_ID];
        }

        if (client_obj.contains(KEY_CLIENT_NAME) && client_obj[KEY_CLIENT_NAME].is_string()) {
            get_client_properties().name = client_obj[KEY_CLIENT_NAME];
        }

        if (client_obj.contains(KEY_CLIENT_VERSION) && client_obj[KEY_CLIENT_VERSION].is_string()) {
            get_client_properties().version = client_obj[KEY_CLIENT_VERSION];
        }
    }

    static void _ingest_engine_config(nlohmann::json engine_obj) {
        if (engine_obj.contains(KEY_ENGINE_MODULES) && engine_obj[KEY_ENGINE_MODULES].is_array()) {
            std::vector<std::string> modules;
            for (auto module_obj : engine_obj[KEY_ENGINE_MODULES]) {
                if (module_obj.is_string()) {
                    modules.push_back(module_obj);
                }
            }

            if (modules.size() > 0) {
                set_load_modules(modules);
            }
        }

        if (engine_obj.contains(KEY_ENGINE_RENDER_BACKENDS) && engine_obj[KEY_ENGINE_RENDER_BACKENDS].is_array()) {
            std::vector<std::string> backends;
            for (auto rb_obj : engine_obj[KEY_ENGINE_RENDER_BACKENDS]) {
                if (rb_obj.is_string()) {
                    backends.push_back(rb_obj);
                }
            }

            if (backends.size() > 0) {
                set_render_backends(backends);
            }
        }

        if (engine_obj.contains(KEY_ENGINE_TICKRATE) && engine_obj[KEY_ENGINE_TICKRATE].is_number_integer()) {
            set_target_tickrate(engine_obj[KEY_ENGINE_TICKRATE]);
        }

        if (engine_obj.contains(KEY_ENGINE_FRAMERATE) && engine_obj[KEY_ENGINE_TICKRATE].is_number_integer()) {
            set_target_framerate(engine_obj[KEY_ENGINE_FRAMERATE]);
        }
    }

    static void _ingest_window_config(nlohmann::json window_obj) {
        InitialWindowParameters win_params;
        if (window_obj.contains(KEY_WINDOW_ID) && window_obj[KEY_WINDOW_ID].is_string()) {
            win_params.id = window_obj[KEY_WINDOW_ID];
        }
        
        if (window_obj.contains(KEY_WINDOW_TITLE) && window_obj[KEY_WINDOW_TITLE].is_string()) {
            win_params.title = window_obj[KEY_WINDOW_TITLE];
        }

        if (window_obj.contains(KEY_WINDOW_MODE) && window_obj[KEY_WINDOW_MODE].is_string()) {
            win_params.mode = window_obj[KEY_WINDOW_MODE];
        }

        if (window_obj.contains(KEY_WINDOW_VSYNC) && window_obj[KEY_WINDOW_VSYNC].is_boolean()) {
            win_params.vsync = window_obj[KEY_WINDOW_VSYNC];
        }

        if (window_obj.contains(KEY_WINDOW_MOUSE) && window_obj[KEY_WINDOW_MOUSE].is_object()) {
            auto mouse_obj = window_obj[KEY_WINDOW_MOUSE];

            if (mouse_obj.contains(KEY_WINDOW_MOUSE_VISIBLE) && mouse_obj[KEY_WINDOW_MOUSE_VISIBLE].is_boolean()) {
                win_params.mouse_visible = mouse_obj[KEY_WINDOW_MOUSE_VISIBLE];
            }

            if (mouse_obj.contains(KEY_WINDOW_MOUSE_CAPTURE) && mouse_obj[KEY_WINDOW_MOUSE_CAPTURE].is_boolean()) {
                win_params.mouse_captured = mouse_obj[KEY_WINDOW_MOUSE_CAPTURE];
            }
        }

        if (window_obj.contains(KEY_WINDOW_POSITION) && window_obj[KEY_WINDOW_POSITION].is_object()) {
            auto pos_obj = window_obj[KEY_WINDOW_POSITION];

            if (pos_obj.contains(KEY_WINDOW_POSITION_X) && pos_obj[KEY_WINDOW_POSITION_X].is_number_integer()) {
                win_params.position.x = pos_obj[KEY_WINDOW_POSITION_X];
            }

            if (pos_obj.contains(KEY_WINDOW_POSITION_Y) && pos_obj[KEY_WINDOW_POSITION_Y].is_number_integer()) {
                win_params.position.y = pos_obj[KEY_WINDOW_POSITION_Y];
            }
        }

        if (window_obj.contains(KEY_WINDOW_DIMENSIONS) && window_obj[KEY_WINDOW_DIMENSIONS].is_object()) {
            auto dim_obj = window_obj[KEY_WINDOW_DIMENSIONS];

            if (dim_obj.contains(KEY_WINDOW_DIMENSIONS_W) && dim_obj[KEY_WINDOW_DIMENSIONS_W].is_number_integer()) {
                win_params.dimensions.x = dim_obj[KEY_WINDOW_DIMENSIONS_W];
            }

            if (dim_obj.contains(KEY_WINDOW_DIMENSIONS_H) && dim_obj[KEY_WINDOW_DIMENSIONS_H].is_number_integer()) {
                win_params.dimensions.y = dim_obj[KEY_WINDOW_DIMENSIONS_H];
            }
        }

        set_initial_window_parameters(win_params);
    }

    static void _ingest_bindings_config(nlohmann::json bindings_obj) {
        if (bindings_obj.contains(KEY_BINDINGS_DEFAULT_BINDINGS_RESOURCE)
                && bindings_obj[KEY_BINDINGS_DEFAULT_BINDINGS_RESOURCE].is_string()) {
            set_default_bindings_resource_id(bindings_obj[KEY_BINDINGS_DEFAULT_BINDINGS_RESOURCE]);
        }

        if (bindings_obj.contains(KEY_BINDINGS_SAVE_USER_BINDINGS)
                && bindings_obj[KEY_BINDINGS_SAVE_USER_BINDINGS].is_boolean()) {
            set_save_user_bindings(bindings_obj[KEY_BINDINGS_SAVE_USER_BINDINGS]);
        }
    }

    static bool _ingest_config_content(std::istream &stream) {
        nlohmann::json json_root = nlohmann::json::parse(stream, nullptr, true, true);

        if (json_root.contains(KEY_CLIENT)) {
            _ingest_client_properties(json_root.at(KEY_CLIENT));
        }

        if (json_root.contains(KEY_ENGINE)) {
            _ingest_engine_config(json_root.at(KEY_ENGINE));
        }

        if (json_root.contains(KEY_WINDOW)) {
            _ingest_window_config(json_root.at(KEY_WINDOW));
        }

        if (json_root.contains(KEY_BINDINGS)) {
            _ingest_bindings_config(json_root.at(KEY_BINDINGS));
        }

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

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

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

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
    /*static inline bool _ends_with(const std::string &haystack, const std::string &needle)
    {
        if (needle.size() > haystack.size()) {
            return false;
        }
        return std::equal(needle.rbegin(), needle.rend(), haystack.rbegin());
    }*/

    static std::filesystem::path _get_resources_path(void) {
        return std::filesystem::current_path() / RESOURCES_DIR;
    }

    static std::optional<std::string> _get_json_string(nlohmann::json root, std::string key) {
        if (root.contains(key) && root[key].is_string()) {
            return root[key];
        } else {
            return nullptr;
        }
    }

    static std::optional<unsigned int> _get_json_uint(nlohmann::json root, std::string key) {
        if (root.contains(key) && root[key].is_number_integer()) {
            return root[key];
        } else {
            return std::make_optional<unsigned int>();
        }
    }

    static std::optional<int> _get_json_int(nlohmann::json root, std::string key) {
        if (root.contains(key) && root[key].is_number_integer()) {
            return root[key];
        } else {
            return std::make_optional<int>();
        }
    }

    static std::optional<bool> _get_json_bool(nlohmann::json root, std::string key) {
        if (root.contains(key) && root[key].is_boolean()) {
            return root[key];
        } else {
            return std::make_optional<bool>();
        }
    }

    static void _ingest_client_properties(nlohmann::json client_obj) {
        if (auto id = _get_json_string(client_obj, KEY_CLIENT_ID); id.has_value()) {
            get_client_properties().id = id.value();
        }

        if (auto name = _get_json_string(client_obj, KEY_CLIENT_NAME); name.has_value()) {
            get_client_properties().name = name.value();
        }

        if (auto version = _get_json_string(client_obj, KEY_CLIENT_VERSION); version.has_value()) {
            get_client_properties().version = version.value();
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

        if (auto tickrate = _get_json_uint(engine_obj, KEY_ENGINE_TICKRATE); tickrate.has_value()) {
            set_target_tickrate(tickrate.value());
        }

        if (auto framerate = _get_json_uint(engine_obj, KEY_ENGINE_FRAMERATE); framerate.has_value()) {
            set_target_framerate(framerate.value());
        }
    }

    static void _ingest_window_config(nlohmann::json window_obj) {
        InitialWindowParameters win_params;
        if (auto id = _get_json_string(window_obj, KEY_WINDOW_ID); id.has_value()) {
            win_params.id = id.value();
        }
        
        if (auto title = _get_json_string(window_obj, KEY_WINDOW_TITLE); title.has_value()) {
            win_params.title = title.value();
        }

        if (auto mode = _get_json_string(window_obj, KEY_WINDOW_MODE); mode.has_value()) {
            win_params.mode = mode.value();
        }

        if (auto vsync = _get_json_bool(window_obj, KEY_WINDOW_VSYNC); vsync.has_value()) {
            win_params.vsync = vsync.value();
        }

        if (window_obj.contains(KEY_WINDOW_MOUSE) && window_obj[KEY_WINDOW_MOUSE].is_object()) {
            auto mouse_obj = window_obj[KEY_WINDOW_MOUSE];

            if (auto visible = _get_json_bool(mouse_obj, KEY_WINDOW_MOUSE_VISIBLE); visible.has_value()) {
                win_params.mouse_visible = visible.value();
            }

            if (auto capture = _get_json_bool(mouse_obj, KEY_WINDOW_MOUSE_CAPTURE); capture.has_value()) {
                win_params.mouse_captured = capture.value();
            }
        }

        if (window_obj.contains(KEY_WINDOW_POSITION) && window_obj[KEY_WINDOW_POSITION].is_object()) {
            auto pos_obj = window_obj[KEY_WINDOW_POSITION];

            if (auto pos_x = _get_json_int(pos_obj, KEY_WINDOW_POSITION_X); pos_x.has_value()) {
                win_params.position.x = pos_x.value();
            }

            if (auto pos_y = _get_json_int(pos_obj, KEY_WINDOW_POSITION_Y); pos_y.has_value()) {
                win_params.position.y = pos_y.value();
            }
        }

        if (window_obj.contains(KEY_WINDOW_DIMENSIONS) && window_obj[KEY_WINDOW_DIMENSIONS].is_object()) {
            auto dim_obj = window_obj[KEY_WINDOW_DIMENSIONS];

            if (auto dim_w = _get_json_uint(dim_obj, KEY_WINDOW_DIMENSIONS_W); dim_w.has_value()) {
                win_params.dimensions.x = dim_w.value();
            }

            if (auto dim_h = _get_json_uint(dim_obj, KEY_WINDOW_DIMENSIONS_H); dim_h.has_value()) {
                win_params.dimensions.y = dim_h.value();
            }
        }

        set_initial_window_parameters(win_params);
    }

    static void _ingest_bindings_config(nlohmann::json bindings_obj) {
        if (auto res_id = _get_json_string(bindings_obj, KEY_BINDINGS_DEFAULT_BINDINGS_RESOURCE); res_id.has_value()) {
            set_default_bindings_resource_id(res_id.value());
        }

        if (auto save = _get_json_bool(bindings_obj, KEY_BINDINGS_SAVE_USER_BINDINGS); save.has_value()) {
            set_save_user_bindings(save.value());
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
        auto config_path = _get_resources_path() / ns / CONFIG_FILE_NAME;

        if (!std::filesystem::is_regular_file(config_path)) {
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

        std::vector<std::filesystem::path> candidate_packages;

        for (const auto &child : std::filesystem::directory_iterator(res_dir)) {
            if (child.path().extension() != EXTENSION_SEPARATOR ARP_EXT) {
                continue;
            }

            if (!arp_is_base_archive(child.path().string().c_str())) {
                continue;
            }

            arp_package_meta_t meta;
            auto rc = arp_load_from_file(child.path().string().c_str(), &meta, nullptr);
            if (rc != 0) {
                _ARGUS_WARN("Failed to load package %s while searching for config (libarp says: %s)",
                    child.path().filename().c_str(), arp_get_error());
                continue;
            }

            if (strcmp(meta.package_namespace, ns.c_str()) == 0) {
                candidate_packages.push_back(child);
            }
        }

        for (const auto &candidate : candidate_packages) {
            auto filename = candidate.filename().c_str();

            _ARGUS_DEBUG("Searching for client config in package %s (namespace matches)", filename);

            ArpPackage pack;
            auto rc = arp_load_from_file(candidate.string().c_str(), nullptr, &pack);
            if (rc != 0) {
                _ARGUS_WARN("Failed to load package at %s while searching for config (libarp says: %s)",
                    filename, arp_get_error());
                continue;
            }

            arp_resource_meta_t res_meta;
            rc = arp_find_resource(pack, (ns + ARP_NS_SEPARATOR + CONFIG_BASE_NAME).c_str(), &res_meta);
            if (rc == E_ARP_RESOURCE_NOT_FOUND) {
                _ARGUS_DEBUG("Did not find config in package %s", candidate.filename().c_str());
                continue;
            } else if (rc != 0) {
                _ARGUS_WARN("Failed to find config in package %s (libarp says: %s)", filename, arp_get_error());
                continue;
            }

            if (strcmp(res_meta.media_type, CONFIG_MEDIA_TYPE) != 0) {
                _ARGUS_WARN("File \"%s\" in package %s has unexpected media type %s, cannot load as client config",
                    CONFIG_BASE_NAME, filename, res_meta.media_type);
                continue;
            }

            assert(rc == 0);

            auto *res = arp_load_resource(&res_meta);
            if (res == nullptr) {
                _ARGUS_WARN("Failed to load config from package %s (libarp says: %s)", filename, arp_get_error());
                continue;
            }

            IMemStream stream(res->data, res->meta.size);
            _ingest_config_content(stream);

            _ARGUS_INFO("Successfully loaded config from package at %s", filename);
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

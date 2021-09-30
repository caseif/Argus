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

// module lowlevel
#include "argus/lowlevel/filesystem.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/module.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/module.hpp"
#include "internal/core/module_core.hpp"
#include "internal/core/module_defs.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <cerrno>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <dlfcn.h>
#endif

namespace argus {
    static std::map<const std::string, DynamicModule> g_dyn_module_registrations;
    static std::vector<DynamicModule> g_sorted_dyn_modules;

    static std::vector<StaticModule> g_enabled_static_modules;
    static std::map<const std::string, DynamicModule> g_enabled_dyn_modules_staging;
    static std::vector<DynamicModule> g_enabled_dyn_modules;

    static std::string _locate_dynamic_module(const std::string &id) {
        std::string modules_dir_path = get_parent(get_executable_path()) + PATH_SEPARATOR MODULES_DIR_NAME;

        if (!is_directory(modules_dir_path)) {
            _ARGUS_WARN("Dynamic module directory not found.\n");
            return "";
        }

        std::string module_path = modules_dir_path + PATH_SEPARATOR
                + SHARED_LIB_PREFIX + id + EXTENSION_SEPARATOR + SHARED_LIB_EXT;
        if (!is_regfile(module_path)) {
            _ARGUS_WARN("Item referred to by %s is not a regular file or is inaccessible\n", module_path.c_str());
            return "";
        }

        return module_path;
    }

    std::map<std::string, std::string> get_present_dynamic_modules(void) {
        std::string modules_dir_path = get_parent(get_executable_path()) + PATH_SEPARATOR MODULES_DIR_NAME;

        if (!is_directory(modules_dir_path)) {
            _ARGUS_INFO("No dynamic modules to load.\n");
            return std::map<std::string, std::string>();
        }

        std::vector<std::string> entries = list_directory_entries(modules_dir_path);
        if (entries.empty()) {
            _ARGUS_INFO("No dynamic modules to load.\n");
            return std::map<std::string, std::string>();
        }

        std::map<std::string, std::string> modules;

        for (const auto &filename : entries) {
            std::string full_path = modules_dir_path + PATH_SEPARATOR + filename;

            if (!is_regfile(full_path)) {
                _ARGUS_DEBUG("Ignoring non-regular module file %s\n", full_path.c_str());
                continue;
            }

            if (SHARED_LIB_PREFIX[0] != '\0' && filename.find(SHARED_LIB_PREFIX) != 0) {
                _ARGUS_DEBUG("Ignoring module file %s with invalid prefix\n", filename.c_str());
                continue;
            }

            std::string ext;
            size_t ext_sep_index = filename.rfind(EXTENSION_SEPARATOR);
            if (ext_sep_index != std::string::npos) {
                ext = filename.substr(ext_sep_index + 1);
            }

            if (ext != SHARED_LIB_EXT) {
                _ARGUS_WARN("Ignoring module file %s with invalid extension\n", filename.c_str());
                continue;
            }

            auto base_name = filename.substr(std::strlen(SHARED_LIB_PREFIX),
                    ext_sep_index - std::strlen(SHARED_LIB_PREFIX));
            modules[base_name] = full_path;
        }

        return modules;
    }

    template <typename T>
    static std::vector<std::string> _topo_sort(const std::vector<T> nodes,
            const std::vector<std::pair<T, T>> edges) {
        std::vector<T> sorted_nodes;
        std::deque<T> start_nodes;
        std::vector<std::pair<T, T>> remaining_edges(edges);

        for (auto &node : nodes) {
            start_nodes.push_back(node);
        }

        for (auto &edge : remaining_edges) {
            start_nodes.erase(std::find(start_nodes.begin(), start_nodes.end(), edge.second));
        }

        while (!start_nodes.empty()) {
            auto &cur_node = start_nodes.front();
            start_nodes.pop_front();
            if (std::count(sorted_nodes.begin(), sorted_nodes.end(), cur_node) == 0) {
                sorted_nodes.push_back(cur_node);
            }

            std::vector<std::pair<T, T>> remove_edges;
            for (auto &cur_edge : remaining_edges) {
                auto &dest_node = cur_edge.second;
                if (cur_edge.first != cur_node) {
                    continue;
                }

                remove_edges.push_back(cur_edge);

                bool has_incoming_edges = false;
                for (auto &check_edge : remaining_edges) {
                    if (check_edge != cur_edge && check_edge.second == dest_node) {
                        has_incoming_edges = true;
                        break;
                    }
                }

                if (!has_incoming_edges) {
                    start_nodes.push_back(dest_node);
                }
            }

            for (auto &edge : remove_edges) {
                remaining_edges.erase(std::find(remaining_edges.begin(), remaining_edges.end(), edge));
            }
        }

        if (!remaining_edges.empty()) {
            throw std::invalid_argument("Graph contains cycles");
        }

        return sorted_nodes;
    }

    static std::vector<DynamicModule> _topo_sort_modules(const std::map<const std::string, DynamicModule> module_map) {
        std::vector<std::string> module_ids;
        std::vector<std::pair<std::string, std::string>> edges;

        for (auto &mod_pair : module_map) {
            module_ids.push_back(mod_pair.first);
            for (auto &dep : mod_pair.second.dependencies) {
                if (module_map.find(dep) != module_map.end()) {
                    edges.push_back({dep, mod_pair.first});
                }
            }
        }

        std::vector<DynamicModule> sorted_modules;
        try {
            auto sorted_ids = _topo_sort(module_ids, edges);
            for (auto &id : sorted_ids) {
                sorted_modules.push_back(module_map.find(id)->second);
            }
        } catch (std::invalid_argument) {
            _ARGUS_FATAL("Circular dependency detected in dynamic modules, cannot proceed.\n");
        }

        return sorted_modules;
    }

    static std::string _format_load_error(std::string msg, std::vector<std::string> dependent_chain) {
        std::stringstream ss;
        ss << msg << "\n";
        for (const auto &dependent : dependent_chain) {
            ss << "    Required by module \"" << dependent << "\"\n";
        }
        return ss.str();
    }

    static DynamicModule &_load_dynamic_module(std::string id,
            std::vector<std::string> dependent_chain = {}) {
        auto path = _locate_dynamic_module(id);
        if (path == "") {
            _ARGUS_FATAL("%s", _format_load_error("Dynamic module " + id + " was requested but could not be located",
                    dependent_chain).c_str());
        }

        _ARGUS_DEBUG("%s", _format_load_error("Attempting to load dynamic module " + id + " from file " + path,
                dependent_chain).c_str());

        void *handle = nullptr;
        #ifdef _WIN32
        handle = LoadLibraryA(path.c_str());
        if (handle == nullptr) {
            auto err_msg = _format_load_error("Failed to load dynamic module " + id
                    + " (error " + std::to_string(GetLastError()) + ")", dependent_chain);
            _ARGUS_FATAL("%s", err_msg.c_str());
        }
        #else
        handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (handle == nullptr) {
            auto err_msg = _format_load_error("Failed to load dynamic module " + id
                    + " (error: " + dlerror() + ")", dependent_chain);
            _ARGUS_FATAL("%s", err_msg.c_str());
        }
        #endif

        if (g_dyn_module_registrations.find(id) == g_dyn_module_registrations.end()) {
            auto err_msg = _format_load_error("Module " + id
                    + " attempted to register itself by a different ID than indicated by its filename",
                    dependent_chain);
            _ARGUS_FATAL("%s", err_msg.c_str());
        }

        auto &mod = g_dyn_module_registrations.find(id)->second;
        mod.handle = handle;
        return mod;
    }

    static void _enable_dynamic_module(const std::string &module_id,
            const std::vector<std::string> &dependent_chain = {}) {
        if (g_enabled_dyn_modules_staging.find(module_id) != g_enabled_dyn_modules_staging.end()) {
            return;
        }

        // skip duplicates
        for (const auto &enabled_module : g_enabled_dyn_modules_staging) {
            if (enabled_module.first == module_id) {
                if (dependent_chain.empty()) {
                    _ARGUS_WARN("Module \"%s\" requested more than once.\n", module_id.c_str());
                }
                return;
            }
        }

        auto it = g_dyn_module_registrations.find(module_id);
        if (it == g_dyn_module_registrations.cend()) {
            auto loaded_modules = _load_dynamic_module(module_id);

            it = g_dyn_module_registrations.find(module_id);
            if (it == g_dyn_module_registrations.cend()) {
                std::stringstream err_msg;
                err_msg << "Module \"" << module_id
                        << "\" was loaded but a matching registration was not found (name mismatch?)";
                for (const auto &dependent : dependent_chain) {
                    err_msg << "\n    Required by module \"" << dependent << "\"";
                }
                throw std::invalid_argument(err_msg.str());
            }
        }

        std::vector<std::string> new_chain = dependent_chain;
        new_chain.insert(new_chain.cend(), module_id);
        for (const auto &dependency : it->second.dependencies) {
            // skip static modules since they're always loaded
            if (g_static_module_ids.find(dependency) != g_static_module_ids.end()) {
                continue;
            }

            _enable_dynamic_module(dependency, new_chain);
        }

        g_enabled_dyn_modules_staging.insert({module_id, it->second});

        _ARGUS_INFO("Enabled dynamic module %s.\n", module_id.c_str());
    }

    void enable_dynamic_module(const std::string &module_id) {
        _enable_dynamic_module(module_id);
    }

    void enable_modules(const std::vector<std::string> &modules) {
        std::set<std::string> all_modules; // requested + transitive modules

        for (const auto &module_id : modules) {
            // Important: MSVC uses iterators for std::array whereas GCC/Clang
            // take a shortcut and use a pointer directly, so the `auto` type
            // here is actually different per-platform. For instance, forcing it
            // to be a pointer breaks MSVC compilation.
            auto found_static = std::find_if(g_static_modules.cbegin(), g_static_modules.cend(),
                [module_id](auto &sm) { return sm.id == module_id; });
            if (found_static != g_static_modules.cend()) {
                all_modules.insert({found_static->id});
                all_modules.insert(found_static->dependencies.begin(), found_static->dependencies.end());
            } else {
                enable_dynamic_module(module_id);
            }
        }

        // we add them to the master list like this in order to preserve the hardcoded load order
        for (const auto &mod : g_static_modules) {
            if (std::count(all_modules.begin(), all_modules.end(), mod.id) != 0) {
                g_enabled_static_modules.push_back(mod);
            }
        }

        // we'll sort the dynamic modules just before bringing them up, since
        // they can still be requested during the Load lifecycle event
    }

    void unload_dynamic_modules(void) {
        for (const auto &mod : g_dyn_module_registrations) {
            auto handle = mod.second.handle;
            #ifdef _WIN32
            if (FreeLibrary(reinterpret_cast<HMODULE>(handle)) == 0) {
                _ARGUS_WARN("Failed to unload dynamic module (errno: %d)\n", GetLastError());
            }
            #else
            if (dlclose(handle) != 0) {
                _ARGUS_WARN("Failed to unload dynamic module (errno: %d)\n", errno);
            }
            #endif
        }
    }

    void register_dynamic_module(const std::string &id, LifecycleUpdateCallback lifecycle_callback,
            std::initializer_list<std::string> dependencies) {
        if (std::count(g_static_module_ids.begin(), g_static_module_ids.end(), id)) {
            throw std::invalid_argument("Module identifier is already in use by static module: " + id);
        }

        if (g_dyn_module_registrations.find(id) != g_dyn_module_registrations.cend()) {
            throw std::invalid_argument("Module is already registered: " + id);
        }

        for (char ch : id) {
            if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch == '_'))) {
                throw std::invalid_argument("Invalid module identifier: " + id);
            }
        }

        auto mod = DynamicModule { id, lifecycle_callback, dependencies, nullptr };

        g_dyn_module_registrations.insert({ id, std::move(mod) });

        _ARGUS_DEBUG("Registered dynamic module %s\n", id.c_str());
    }

    static void _send_lifecycle_update(LifecycleStage stage) {
        for (const auto &mod : g_enabled_static_modules) {
            mod.lifecycle_update_callback(stage);
        }

        for (const auto &mod : g_enabled_dyn_modules) {
            mod.lifecycle_update_callback(stage);
        }
    }

    void init_modules(void) {
        auto dyn_mod_initial_count = g_enabled_dyn_modules_staging.size();
        g_enabled_dyn_modules = _topo_sort_modules(g_enabled_dyn_modules_staging);

        _ARGUS_DEBUG("Propagating Load lifecycle stage\n");
        // give modules a chance to request additional dynamic modules
        _send_lifecycle_update(LifecycleStage::Load);
        // re-sort the dynamic module if it was augmented
        if (g_enabled_dyn_modules_staging.size() > dyn_mod_initial_count) {
            _ARGUS_DEBUG("Dynamic module list changed, must re-sort\n");
            g_enabled_dyn_modules = _topo_sort_modules(g_enabled_dyn_modules_staging);
        }

        _ARGUS_DEBUG("Propagating remaining bring-up lifecycle stages\n");

        for (LifecycleStage stage = LifecycleStage::PreInit; stage <= LifecycleStage::PostInit;
                stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            _send_lifecycle_update(stage);
        }
    }

    void deinit_modules(void) {
        for (LifecycleStage stage = LifecycleStage::PreDeinit; stage <= LifecycleStage::PostDeinit;
             stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            for (auto it = g_enabled_static_modules.rbegin(); it != g_enabled_static_modules.rend(); ++it) {
                it->lifecycle_update_callback(stage);
            }

            for (auto it = g_enabled_dyn_modules.rbegin(); it != g_enabled_dyn_modules.rend(); ++it) {
                it->lifecycle_update_callback(stage);
            }
        }
    }
}

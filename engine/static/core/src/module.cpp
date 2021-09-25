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
    std::map<const std::string, DynamicModule> g_dyn_module_registrations;
    std::vector<DynamicModule> g_sorted_dyn_modules;

    std::vector<StaticModule> g_enabled_static_modules;
    std::vector<DynamicModule> g_enabled_dyn_modules;

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

    static std::set<std::string> _resolve_transitive_dependencies(const std::set<std::string> initial,
            const std::vector<std::string> &blame_chain) {
        std::set<std::string> target;

        for (const auto &module_id : initial) {
            target.insert(module_id);

            const auto *found_static = std::find_if(g_static_modules.begin(), g_static_modules.end(),
                    [module_id](auto &sm) { return sm.id == module_id; });
            if (found_static != g_static_modules.end()) {
                target.insert(found_static->dependencies.begin(), found_static->dependencies.end());
                // no need to search recursively since static modules are
                // guaranteed to have a complete dependency list
                continue;
            }

            const auto &found_dyn = g_dyn_module_registrations.find(module_id);
            if (found_dyn != g_dyn_module_registrations.end()) {
                target.insert(found_dyn->second.dependencies.begin(), found_dyn->second.dependencies.end());

                std::vector<std::string> new_blame_chain(blame_chain);
                new_blame_chain.push_back(module_id);
                auto trans_deps = _resolve_transitive_dependencies(found_dyn->second.dependencies, new_blame_chain);
                target.insert(trans_deps.begin(), trans_deps.end());

                continue;
            }

            std::stringstream blame_str;
            for (auto &dependency : blame_chain) {
                blame_str << dependency << " -> ";
            }
            blame_str << module_id;

            _ARGUS_FATAL("Failed to resolve transitive dependency \"%s\" (dependency chain: %s)\n", module_id.c_str(),
                    blame_str.str().c_str());
        }

        return target;
    }

    void load_dynamic_modules(void) {
        //TODO: we really shouldn't be loading modules that aren't explicitly requested
        auto present_modules = get_present_dynamic_modules();

        for (const auto &mod : present_modules) {
            const auto &module_id = mod.first;
            const auto &so_path = mod.second;

            _ARGUS_INFO("Found dynamic module %s as file %s, attempting to load.\n",
                    module_id.c_str(), so_path.c_str());

            void *handle = nullptr;
            #ifdef _WIN32
            handle = LoadLibraryA(so_path.c_str());
            if (handle == nullptr) {
                _ARGUS_WARN("Failed to load dynamic module %s (errno: %d)\n", module_id.c_str(), GetLastError());
                continue;
            }
            #else
            handle = dlopen(so_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (handle == nullptr) {
                _ARGUS_WARN("Failed to load dynamic module %s (error: %s)\n", module_id.c_str(), dlerror());
                continue;
            }
            #endif

            g_dyn_module_registrations.find(module_id)->second.handle = handle;
        }

        //TODO: do topo. sort before populating list
        for (const auto &mod : g_dyn_module_registrations) {
            g_sorted_dyn_modules.push_back(mod.second);
        }
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

    void enable_modules(const std::vector<std::string> &modules) {
        std::set<std::string> all_modules; // requested + transitive modules

        for (const auto &module_id : modules) {
            auto *found_static = std::find_if(g_static_modules.begin(), g_static_modules.end(),
                [module_id](auto &sm) { return sm.id == module_id; });
            if (found_static != g_static_modules.end()) {
                all_modules.insert(found_static->dependencies.begin(), found_static->dependencies.end());
            } else {
                auto found_dyn = g_dyn_module_registrations.find(module_id);
                if (found_dyn != g_dyn_module_registrations.end()) {
                    auto trans_deps = _resolve_transitive_dependencies(found_dyn->second.dependencies, { module_id });
                    all_modules.insert(trans_deps.begin(), trans_deps.end());
                } else {
                    _ARGUS_FATAL("Module %s was requested but could not be found.\n", module_id.c_str());
                }
            }
        }

        // we add them to the master list like this in order to preserve the hardcoded load order
        for (const auto &mod : g_static_modules) {
            if (std::count(all_modules.begin(), all_modules.end(), mod.id) != 0) {
                g_enabled_static_modules.push_back(mod);
            }
        }

        for (const auto &mod : g_sorted_dyn_modules) {
            if (std::count(all_modules.begin(), all_modules.end(), mod.id) != 0) {
                g_enabled_dyn_modules.push_back(mod);
            }
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

        _ARGUS_INFO("Registered module %s\n", id.c_str());
    }

    static void _enable_dynamic_module(const std::string &module_id, const std::vector<std::string> &dependent_chain) {
        // skip duplicates
        for (const auto &enabled_module : g_enabled_dyn_modules) {
            if (enabled_module.id == module_id) {
                if (dependent_chain.empty()) {
                    _ARGUS_WARN("Module \"%s\" requested more than once.\n", module_id.c_str());
                }
                return;
            }
        }

        auto it = g_dyn_module_registrations.find(module_id);
        if (it == g_dyn_module_registrations.cend()) {
            std::stringstream err_msg;
            err_msg << "Module \"" << module_id << "\" was requested, but is not registered";
            for (const auto &dependent : dependent_chain) {
                err_msg << "\n    Required by module \"" << dependent << "\"";
            }
            throw std::invalid_argument(err_msg.str());
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

        g_enabled_dyn_modules.push_back(it->second);

        _ARGUS_INFO("Enabled module %s.\n", module_id.c_str());
    }

    void enable_dynamic_module(const std::string &module_id) {
        _enable_dynamic_module(module_id, {});
    }

    void init_modules(void) {
        for (LifecycleStage stage = LifecycleStage::Load; stage <= LifecycleStage::PostInit;
                stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            for (const auto &mod : g_enabled_static_modules) {
                mod.lifecycle_update_callback(stage);
            }

            for (const auto &mod_info : g_enabled_dyn_modules) {
                mod_info.lifecycle_update_callback(stage);
            }
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

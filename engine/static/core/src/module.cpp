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
    std::map<const std::string, DynamicModule> g_registered_dyn_modules;

    std::vector<StaticModule> g_enabled_static_modules;
    std::vector<DynamicModule> g_enabled_dyn_modules;

    void init_static_modules(void) {
        for (auto &module : g_static_modules) {
            module.init_callback();
        }
    }

    std::map<std::string, std::string> get_present_external_modules(void) {
        std::string modules_dir_path = get_parent(get_executable_path()) + PATH_SEPARATOR MODULES_DIR_NAME;

        if (!is_directory(modules_dir_path)) {
            _ARGUS_INFO("No external modules to load.\n");
            return std::map<std::string, std::string>();
        }

        std::vector<std::string> entries = list_directory_entries(modules_dir_path);
        if (entries.empty()) {
            _ARGUS_INFO("No external modules to load.\n");
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

    void load_external_modules(void) {
        auto modules = get_present_external_modules();

        for (const auto &module : modules) {
            auto &module_id = module.first;
            auto &so_path = module.second;

            _ARGUS_INFO("Found external module %s as file %s, attempting to load.\n",
                    module_id.c_str(), so_path.c_str());

            void *handle = nullptr;
            #ifdef _WIN32
            handle = LoadLibraryA(so_path.c_str());
            if (handle == nullptr) {
                _ARGUS_WARN("Failed to load external module %s (errno: %d)\n", module_id.c_str(), GetLastError());
                continue;
            }
            #else
            handle = dlopen(so_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (handle == nullptr) {
                _ARGUS_WARN("Failed to load external module %s (error: %s)\n", module_id.c_str(), dlerror());
                continue;
            }
            #endif

            g_registered_dyn_modules.find(module_id)->second.handle = handle;
        }
    }

    void unload_external_modules(void) {
        for (auto &mod : g_registered_dyn_modules) {
            auto handle = mod.second.handle;
            #ifdef _WIN32
            if (FreeLibrary(reinterpret_cast<HMODULE>(handle)) == 0) {
                _ARGUS_WARN("Failed to unload external module (errno: %d)\n", GetLastError());
            }
            #else
            if (dlclose(handle) != 0) {
                _ARGUS_WARN("Failed to unload external module (errno: %d)\n", errno);
            }
            #endif
        }
    }

    void enable_static_modules(const std::vector<std::string> &modules) {
        std::vector<std::string> all_modules = modules;
        for (const auto &mod : g_static_modules) {
            if (std::count(modules.begin(), modules.end(), mod.id) != 0) {
                for (const auto &dep : mod.dependencies) {
                    if (std::count(all_modules.begin(), all_modules.end(), dep) == 0) {
                        all_modules.push_back(dep);
                    }
                }
            }
        }

        // we add them to the master list like this in order to preserve the hardcoded load order
        for (const auto &mod : g_static_modules) {
            if (std::count(all_modules.begin(), all_modules.end(), mod.id) != 0) {
                g_enabled_static_modules.push_back(mod);
            }
        }
    }

    void register_dynamic_module(const std::string &id, LifecycleUpdateCallback lifecycle_callback,
            std::initializer_list<std::string> dependencies) {
        if (g_registered_dyn_modules.find(id) != g_registered_dyn_modules.cend()) {
            throw std::invalid_argument("Module is already registered: " + id);
        }

        for (char ch : id) {
            if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch == '_'))) {
                throw std::invalid_argument("Invalid module identifier: " + id);
            }
        }

        auto mod = DynamicModule { id, lifecycle_callback, dependencies, nullptr };

        g_registered_dyn_modules.insert({ id, std::move(mod) });

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

        auto it = g_registered_dyn_modules.find(module_id);
        if (it == g_registered_dyn_modules.cend()) {
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

    static void _deinitialize_modules(void) {
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

    void deinit_modules(void) {
        _deinitialize_modules();

        unload_external_modules();
    }
}

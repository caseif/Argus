/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/filesystem.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/module.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/module.hpp"
#include "internal/core/module_core.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
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
    // external module lifecycle hooks
    extern void init_module_wm(void);
    extern void init_module_ecs(void);
    extern void init_module_input(void);
    extern void init_module_resman(void);
    extern void init_module_render(void);

    std::map<const std::string, const NullaryCallback> g_stock_module_initializers{{MODULE_CORE, init_module_core},
                                                                                   {MODULE_WM, init_module_wm},
                                                                                   {MODULE_ECS, init_module_ecs},
                                                                                   {MODULE_INPUT, init_module_input},
                                                                                   {MODULE_RESMAN, init_module_resman},
                                                                                   {MODULE_RENDER, init_module_render}};

    std::map<const std::string, const ArgusModule> g_registered_modules;

    std::set<ArgusModule, bool (*)(const ArgusModule, const ArgusModule)> g_enabled_modules(
        [](const ArgusModule a, const ArgusModule b) {
            if (a.layer != b.layer) {
                return a.layer < b.layer;
            } else {
                return a.id.compare(b.id) < 0;
            }
        });

    static std::vector<void *> g_external_module_handles;

    void init_stock_modules(void) {
        for (auto it = g_stock_module_initializers.cbegin(); it != g_stock_module_initializers.cend(); it++) {
            it->second();
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

        for (auto filename : entries) {
            std::string full_path = modules_dir_path + PATH_SEPARATOR + filename;

            if (!is_regfile(full_path)) {
                _ARGUS_DEBUG("Ignoring non-regular module file %s\n", full_path.c_str());
                continue;
            }

            if (SHARED_LIB_PREFIX != "" && filename.find(SHARED_LIB_PREFIX) != 0) {
                _ARGUS_DEBUG("Ignoring module file %s with invalid prefix\n", filename.c_str());
                continue;
            }

            std::string ext = "";
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

        for (auto module : modules) {
            _ARGUS_INFO("Found external module %s as file %s, attempting to load.\n",
                    module.first.c_str(), module.second.c_str());

            void *handle;
            #ifdef _WIN32
            handle = LoadLibraryA(module.second.c_str());
            if (handle == nullptr) {
                _ARGUS_WARN("Failed to load external module %s (errno: %d)\n", module.first.c_str(), GetLastError());
                continue;
            }
            #else
            handle = dlopen(module.second.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (handle == nullptr) {
                _ARGUS_WARN("Failed to load external module %s (error: %s)\n", module.first.c_str(), dlerror());
                continue;
            }
            #endif

            g_external_module_handles.insert(g_external_module_handles.begin(), handle);
        }
    }

    void unload_external_modules(void) {
        for (void *handle : g_external_module_handles) {
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

    void load_modules(const std::vector<std::string> &modules) {
        for (const std::string module : modules) {
            enable_module(module);
        }
    }

    void register_module(const ArgusModule module) {
        if (g_registered_modules.find(module.id) != g_registered_modules.cend()) {
            ;
            throw std::invalid_argument("Module is already registered: " + module.id);
        }

        for (char ch : module.id) {
            if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch == '_'))) {
                throw std::invalid_argument("Invalid module identifier: " + module.id);
            }
        }

        g_registered_modules.insert({ module.id, module });

        _ARGUS_INFO("Registered module %s\n", module.id.c_str());
    }

    void enable_module(const std::string module_id, const std::vector<std::string> dependent_chain) {
        // skip duplicates
        for (const ArgusModule enabled_module : g_enabled_modules) {
            if (enabled_module.id == module_id) {
                if (dependent_chain.empty()) {
                    _ARGUS_WARN("Module \"%s\" requested more than once.\n", module_id.c_str());
                }
                return;
            }
        }

        auto it = g_registered_modules.find(module_id);
        if (it == g_registered_modules.cend()) {
            std::stringstream err_msg;
            err_msg << "Module \"" << module_id << "\" was requested, but is not registered";
            for (const std::string dependent : dependent_chain) {
                err_msg << "\n    Required by module \"" << dependent << "\"";
            }
            throw std::invalid_argument(err_msg.str());
        }

        std::vector<std::string> new_chain = dependent_chain;
        new_chain.insert(new_chain.cend(), module_id);
        for (const std::string dependency : it->second.dependencies) {
            enable_module(dependency, new_chain);
        }

        g_enabled_modules.insert(it->second);

        _ARGUS_INFO("Enabled module %s.\n", module_id.c_str());
    }

    void enable_module(const std::string module_id) {
        enable_module(module_id, {});
    }

    void _deinitialize_modules(void) {
        for (LifecycleStage stage = LifecycleStage::PRE_DEINIT; stage <= LifecycleStage::POST_DEINIT;
             stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            for (auto it = g_enabled_modules.rbegin(); it != g_enabled_modules.rend(); it++) {
                it->lifecycle_update_callback(stage);
            }
        }
    }

    void deinit_loaded_modules(void) {
        _deinitialize_modules();

        unload_external_modules();
    }
}

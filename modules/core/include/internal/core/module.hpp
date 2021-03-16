/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/core/module.hpp"
#include "internal/core/callback_util.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

#define MODULES_DIR_NAME "modules"
#ifdef _WIN32
    #define SHARED_LIB_PREFIX ""
    #define SHARED_LIB_EXT "dll"
#elif defined(__APPLE__)
    #define SHARED_LIB_PREFIX ""
    #define SHARED_LIB_EXT "dylib"
#else
    #define SHARED_LIB_PREFIX "lib"
    #define SHARED_LIB_EXT "so"
#endif

#define US_PER_S 1000000LLU
#define SLEEP_OVERHEAD_NS 120000LLU

#define RENDER_MODULE_OPENGL "argus_render_opengl"
#define RENDER_MODULE_OPENGLES "argus_render_opengles"
#define RENDER_MODULE_VULKAN "argus_render_vulkan"

namespace argus {
    extern std::map<const std::string, const NullaryCallback> g_stock_module_initializers;

    extern std::map<std::string, NullaryCallback> g_early_init_callbacks;

    extern std::map<const std::string, const ArgusModule> g_registered_modules;

    extern std::set<ArgusModule, bool (*)(const ArgusModule, const ArgusModule)> g_enabled_modules;

    void init_stock_modules(void);
    
    void register_early_init_callback(const std::string module_id, NullaryCallback callback);

    void do_early_init();

    void load_external_modules(void);

    void unload_external_modules(void);

    void load_modules(const std::vector<std::string> &modules);

    std::map<std::string, std::string> get_present_external_modules(void);

    void deinit_loaded_modules(void);
}

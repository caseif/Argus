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

#include "argus/core/module.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/module.hpp"

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

#define US_PER_S 1'000'000ULL
#define SLEEP_OVERHEAD_NS 120'000ns

#define RENDER_MODULE_OPENGL "argus_render_opengl"
#define RENDER_MODULE_OPENGLES "argus_render_opengles"
#define RENDER_MODULE_VULKAN "argus_render_vulkan"

namespace argus {
    struct StaticModule {
        const std::string id;
        const std::set<std::string> dependencies;
        const LifecycleUpdateCallback lifecycle_update_callback;
    };

    void do_early_init();

    void enable_modules(const std::vector<std::string> &modules);
    
    //void load_dynamic_modules(void);

    void unload_dynamic_modules(void);

    std::map<std::string, std::string> get_present_dynamic_modules(void);

    void init_modules(void);

    void deinit_modules(void);
}

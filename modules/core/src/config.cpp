/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module core
#include "argus/core.hpp"
#include "internal/core/config.hpp"

#include <initializer_list>
#include <string>

namespace argus {

    EngineConfig g_engine_config;

    void set_target_tickrate(const unsigned int target_tickrate) {
        g_engine_config.target_tickrate = target_tickrate;
    }

    void set_target_framerate(const unsigned int target_framerate) {
        g_engine_config.target_framerate = target_framerate;
    }

    void set_load_modules(const std::initializer_list<const std::string> module_list) {
        g_engine_config.load_modules = module_list;
    }

}

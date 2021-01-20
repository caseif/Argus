/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module core
#include "argus/core/engine_config.hpp"

#include <string>
#include <vector>

namespace argus {
    struct EngineConfig {
        unsigned int target_tickrate;
        unsigned int target_framerate;
        std::vector<std::string> load_modules;
        std::vector<RenderBackend> render_backends;
        ScreenSpace screen_space;

        EngineConfig(void):
            screen_space(-1, 1, -1, 1) {
        }
    };

    EngineConfig get_engine_config();

}

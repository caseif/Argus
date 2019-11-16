/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

namespace argus {

    typedef struct {
        unsigned int target_tickrate;
        unsigned int target_framerate;
        std::initializer_list<const std::string> load_modules;
    } EngineConfig;

}

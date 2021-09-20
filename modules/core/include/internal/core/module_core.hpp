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

namespace argus {
    extern bool g_core_initializing;
    extern bool g_core_initialized;

    void init_module_core(void);

    void update_lifecycle_core(LifecycleStage stage);
}

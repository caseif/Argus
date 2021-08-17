/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/module.hpp"
#include "internal/core/engine.hpp"
#include "internal/core/module_core.hpp"

#include <string>

namespace argus {
    bool g_core_initializing = false;
    bool g_core_initialized = false;

    static void _update_lifecycle_core(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PreInit:
                _ARGUS_ASSERT(!g_core_initializing && !g_core_initialized, "Cannot initialize engine more than once.");

                g_core_initializing = true;
                break;
            case LifecycleStage::Init:
                g_core_initialized = true;
                break;
            case LifecycleStage::PostDeinit:
                kill_game_thread();

                break;
            default:
                break;
        }
    }

    void init_module_core(void) {
        register_module({ModuleCore, 1, {}, _update_lifecycle_core});
    }
}

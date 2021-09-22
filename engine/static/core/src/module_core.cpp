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
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/module.hpp"
#include "internal/core/engine.hpp"
#include "internal/core/module_core.hpp"

#include <string>

namespace argus {
    bool g_core_initializing = false;
    bool g_core_initialized = false;

    void update_lifecycle_core(LifecycleStage stage) {
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
    }
}

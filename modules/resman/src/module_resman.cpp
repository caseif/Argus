/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module core
#include "argus/core/module.hpp"

// module resman
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/module_resman.hpp"

#include <string>

namespace argus {
    void update_lifecycle_resman(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PostInit:
                ResourceManager::get_global_resource_manager().discover_resources();
                break;
            case LifecycleStage::PostDeinit:
                // not necessary for now since it's static
                //ResourceManager::get_global_resource_manager().~ResourceManager();
                break;
            default:
                break;
        }
    }

    void init_module_resman(void) {
    }
}

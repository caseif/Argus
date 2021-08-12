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

namespace argus {
    static void _update_lifecycle_resman(LifecycleStage stage) {
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
        register_module({ModuleResman, 2, {"core"}, _update_lifecycle_resman});
    }
}

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
    void _update_lifecycle_resman(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::POST_INIT:
                ResourceManager::get_global_resource_manager().discover_resources();
                break;
            default:
                break;
        }
    }

    void init_module_resman(void) {
        register_module({MODULE_RESMAN, 2, {"core"}, _update_lifecycle_resman});
    }
}

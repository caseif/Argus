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

// module ecs
#include "argus/ecs/component_type_registry.hpp"

namespace argus {

    void _update_lifecycle_ecs(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::INIT: {
                // we only accept component registrations during the pre-init stage
                ComponentTypeRegistry::instance()._seal();
                break;
            }
            default: {
                break;
            }
        }
    }

    void init_module_ecs(void) {
        //TODO
    }

}

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

// module ecs
#include "argus/ecs/component_type_registry.hpp"

#include <string>

namespace argus {
    void update_lifecycle_ecs(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
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
    }
}

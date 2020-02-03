/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/core.hpp"

namespace argus {

    static bool g_accepting_component_regs = false;

    void _update_lifecycle_ecs(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PRE_INIT: {
                g_accepting_component_regs = true;
            }
            case LifecycleStage::INIT: {
                // we only accept component registrations during the pre-init stage
                g_accepting_component_regs = false;
            }
        }
    }

    void init_module_ecs(void) {
        //TODO
    }

}

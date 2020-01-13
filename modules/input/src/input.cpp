/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module input
#include "argus/input.hpp"
#include "internal/input/input_helpers.hpp"

// module core
#include "argus/core.hpp"

#include <string>

namespace argus {

    void _update_lifecycle_input(const LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::INIT:
                init_keyboard();
                break;
            default:
                break;
        }
    }

    void init_module_input(void) {
        register_module({MODULE_INPUT, 2, {"core"}, _update_lifecycle_input});
    }

}

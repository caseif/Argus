/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module input
#include "argus/input.hpp"
#include "internal/input_helpers.hpp"

// module core
#include "argus/core.hpp"

namespace argus {

    void update_lifecycle_input(LifecycleStage stage) {
        if (stage == LifecycleStage::INIT) {
            init_keyboard();
        }
    }

}

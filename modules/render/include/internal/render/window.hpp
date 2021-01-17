/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/core.hpp"

// module render
#include "argus/render/window.hpp"

namespace argus {
    void *get_window_handle(const Window &window);

    void window_window_event_callback(const ArgusEvent &event, void *user_data);
}

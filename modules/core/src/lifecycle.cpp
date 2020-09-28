/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/core.hpp"
#include "internal/core/lifecycle.hpp"

#include <map>
#include <string>

namespace argus {
    std::map<std::string, NullaryCallback> g_early_init_callbacks;

    void register_early_init_callback(const std::string module_id, NullaryCallback callback) {
        g_early_init_callbacks.insert({ module_id, callback });
    }
}

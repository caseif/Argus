/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module core
#include "internal/core/dyn_invoke.hpp"

#include <map>
#include <string>

namespace argus {
    static std::map<std::string, void*> dyn_fns;

    void register_module_fn(std::string fn_name, void *addr) {
        dyn_fns.insert({ fn_name, addr });
    }

    void *lookup_module_fn(std::string fn_name) {
        auto it = dyn_fns.find(fn_name);
        if (it != dyn_fns.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }
}
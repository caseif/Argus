/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "internal/core/dyn_invoke.hpp"

#include <map>
#include <string>

namespace argus {
    static std::map<std::string, const void*> dyn_fns;

    void register_module_fn(const std::string &fn_name, const void *addr) {
        dyn_fns.insert({ fn_name, addr });
    }

    const void *lookup_module_fn(const std::string &fn_name) {
        auto it = dyn_fns.find(fn_name);
        if (it != dyn_fns.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }
}

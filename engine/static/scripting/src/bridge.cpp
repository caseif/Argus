/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/functional.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"

#include "argus/scripting/bridge.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <functional>
#include <type_traits>
#include <vector>

#include <cstring>

namespace argus {
    ObjectProxy invoke_native_function_global(const std::string &name, const std::vector<ObjectProxy> &params) {
        auto it = g_registered_fns.find(name);
        if (it == g_registered_fns.cend()) {
            //TODO: throw exception that we can bubble up to the language plugin
            return {};
        }

        std::vector<ObjectProxy> args{ 1 };
        //invoke_native_function(foo, args);

        UNUSED(params);
        //TODO
        return {};
    }

    ObjectProxy invoke_native_function_instance(const std::string &name, const std::string &type_name,
            ObjectProxy instance, const std::vector<ObjectProxy> &params) {
        UNUSED(name);
        UNUSED(type_name);
        UNUSED(instance);
        UNUSED(params);
        //TODO
        return {};
    }
}

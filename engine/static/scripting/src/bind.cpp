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

#include "argus/lowlevel/debug.hpp"

#include "argus/scripting/bind.hpp"
#include "argus/scripting/types.hpp"

#include "internal/scripting/defines.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <algorithm>
#include <set>
#include <string>

#include <cassert>

namespace argus {
    void bind_type(const BoundTypeDef &def) {
        if (g_bound_types.find(def.name) != g_bound_types.cend()) {
            Logger::default_logger().fatal("Script type with name %s has already been bound");
        }

        std::vector<std::string> static_fn_names;
        std::transform(def.static_functions.cbegin(), def.static_functions.cend(), static_fn_names.begin(),
                [](const auto &fn_def) { return fn_def.name; });
        if (std::set(static_fn_names.cbegin(), static_fn_names.cend()).size() != static_fn_names.size()) {
            Logger::default_logger().fatal("Script type with name %s contains duplicate static function definitions");
        }

        std::vector<std::string> instance_fn_names;
        std::transform(def.instance_functions.cbegin(), def.instance_functions.cend(), instance_fn_names.begin(),
                [](const auto &fn_def) { return fn_def.name; });
        if (std::set(instance_fn_names.cbegin(), instance_fn_names.cend()).size() != instance_fn_names.size()) {
            Logger::default_logger().fatal("Script type with name %s contains duplicate instance function definitions");
        }

        // register type
        g_bound_types.insert({ def.name, def });

        // register static functions
        for (const auto &fn : def.static_functions) {
            auto qual_name = def.name + SEPARATOR_STATIC_FN + fn.name;
            assert(g_bound_fns.find(qual_name) != g_bound_fns.cend());
            g_bound_fns.insert({ qual_name, fn });
        }

        // register instance functions
        for (const auto &fn : def.instance_functions) {
            auto qual_name = def.name + SEPARATOR_INSTANCE_FN + fn.name;
            assert(g_bound_fns.find(qual_name) != g_bound_fns.cend());
            g_bound_fns.insert({ qual_name, fn });
        }
    }

    void bind_global_function(const BoundFunctionDef &def) {
        if (g_bound_fns.find(def.name) != g_bound_fns.cend()) {
            Logger::default_logger().fatal("Global function with name %s has already been bound");
        }

        g_bound_fns.insert({ def.name, def });
    }
}

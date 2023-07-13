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

#include "internal/scripting/bind.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <algorithm>
#include <set>
#include <string>

#include <cassert>

namespace argus {
    static void _resolve_param(ObjectType &param_def) {
        if (param_def.type != IntegralType::Pointer && param_def.type != IntegralType::Struct) {
            return;
        }

        assert(param_def.type_index.has_value());
        assert(!param_def.type_name.has_value());

        auto &bound_type = get_bound_type(param_def.type_index.value());
        param_def.type_name = bound_type.name;
    }

    void resolve_parameter_types(BoundTypeDef &type_def) {
        try {
            for (auto &fn : type_def.instance_functions) {
                for (auto &param : fn.second.params) {
                    _resolve_param(param);
                }

                if (fn.second.return_type.type == IntegralType::Pointer
                        || fn.second.return_type.type == IntegralType::Struct) {
                    _resolve_param(fn.second.return_type);
                }
            }

            for (auto &fn : type_def.static_functions) {
                for (auto &param : fn.second.params) {
                    _resolve_param(param);
                }

                if (fn.second.return_type.type == IntegralType::Pointer
                        || fn.second.return_type.type == IntegralType::Struct) {
                    _resolve_param(fn.second.return_type);
                }
            }
        } catch (const std::exception &ex) {
            throw BindingException(type_def.name, ex.what());
        }
    }

    void bind_type(const BoundTypeDef &def) {
        if (g_bound_types.find(def.name) != g_bound_types.cend()) {
            throw BindingException(def.name, "Type with same name has already been bound");
        }

        std::vector<std::string> static_fn_names;
        std::transform(def.static_functions.cbegin(), def.static_functions.cend(),
                std::back_inserter(static_fn_names),
                [](const auto &fn_def) { return fn_def.second.name; });
        if (std::set(static_fn_names.cbegin(), static_fn_names.cend()).size() != static_fn_names.size()) {
            throw BindingException(def.name, "Script type contains duplicate static function definitions");
        }

        std::vector<std::string> instance_fn_names;
        std::transform(def.instance_functions.cbegin(), def.instance_functions.cend(),
                std::back_inserter(instance_fn_names),
                [](const auto &fn_def) { return fn_def.second.name; });
        if (std::set(instance_fn_names.cbegin(), instance_fn_names.cend()).size() != instance_fn_names.size()) {
            throw BindingException(def.name, "Script type contains duplicate instance function definitions");
        }

        g_bound_types.insert({ def.name, def });
        g_bound_type_indices.insert({ def.type_index, def.name });
    }

    void bind_global_function(const BoundFunctionDef &def) {
        if (g_bound_global_fns.find(def.name) != g_bound_global_fns.cend()) {
            throw BindingException(def.name, "Global function with same name has already been bound");
        }

        g_bound_global_fns.insert({ def.name, def });
    }
}

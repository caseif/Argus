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
#include "internal/scripting/util.hpp"
#include "internal/scripting/pimpl/script_context.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>

namespace argus {
    static void _resolve_param(ObjectType &param_def) {
        if (param_def.type == IntegralType::Callback) {
            assert(param_def.callback_type.has_value());
            auto callback_type = param_def.callback_type.value();
            for (auto &subparam : callback_type->params) {
                _resolve_param(subparam);
            }

            _resolve_param(callback_type->return_type);

            return;
        } else if (!is_bound_type(param_def.type)) {
            return;
        }

        assert(param_def.type_index.has_value());
        assert(!param_def.type_name.has_value());

        std::string type_name;
        if (param_def.type == IntegralType::Enum) {
            auto &bound_enum = get_bound_enum(param_def.type_index.value());
            type_name = bound_enum.name;
        } else {
            auto &bound_type = get_bound_type(param_def.type_index.value());

            if (param_def.type == IntegralType::Struct) {
                if (!bound_type.copy_ctor.has_value()) {
                    throw BindingException(bound_type.name,
                            "Struct-typed parameter passed by value with type "
                                    + bound_type.name + " is not copy-constructible");
                }

                if (!bound_type.move_ctor.has_value()) {
                    throw BindingException(bound_type.name,
                            "Struct-typed parameter passed by value with type "
                                    + bound_type.name + " is not move-constructible");
                }

                if (!bound_type.dtor.has_value()) {
                    throw BindingException(bound_type.name,
                            "Struct-typed parameter passed by value with type "
                                    + bound_type.name + " is not destructible");
                }
            }

            type_name = bound_type.name;
        }

        param_def.type_name = type_name;
    }

    static void _resolve_param_types(BoundFunctionDef &fn_def) {
        for (auto &param : fn_def.params) {
            _resolve_param(param);
        }

        _resolve_param(fn_def.return_type);
    }

    void resolve_parameter_types(BoundTypeDef &type_def) {
        try {
            for (auto &fn : type_def.instance_functions) {
                _resolve_param_types(fn.second);
            }

            for (auto &fn : type_def.static_functions) {
                _resolve_param_types(fn.second);
            }
        } catch (const std::exception &ex) {
            throw BindingException(type_def.name, ex.what());
        }
    }

    void resolve_parameter_types(BoundFunctionDef &fn_def) {
        try {
            _resolve_param_types(fn_def);
        } catch (const std::exception &ex) {
            throw BindingException(fn_def.name, ex.what());
        }
    }

    void bind_type(const BoundTypeDef &def) {
        if (g_bound_types.find(def.name) != g_bound_types.cend()) {
            throw BindingException(def.name, "Type with same name has already been bound");
        }

        if (g_bound_global_fns.find(def.name) != g_bound_global_fns.cend()) {
            throw BindingException(def.name, "Global function with same name as type has already been bound");
        }

        if (g_bound_enums.find(def.name) != g_bound_enums.cend()) {
            throw BindingException(def.name, "Enum with same name as type has already been bound");
        }

        //TODO: perform validation on member functions

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

        if (g_bound_types.find(def.name) != g_bound_types.cend()) {
            throw BindingException(def.name, "Type with same name as global function has already been bound");
        }

        if (g_bound_enums.find(def.name) != g_bound_enums.cend()) {
            throw BindingException(def.name, "Enum with same name as global function has already been bound");
        }

        //TODO: perform validation including:
        //  - check that params types aren't garbage
        //  - check that param sizes match types where applicable
        //  - ensure params passed by value are copy-constructible

        g_bound_global_fns.insert({ def.name, def });
    }

    void bind_enum(const BoundEnumDef &def) {
        // check for consistency
        std::unordered_set<uint64_t> ordinals;
        ordinals.reserve(def.values.size());
        std::transform(def.values.cbegin(), def.values.cend(), std::inserter(ordinals, ordinals.end()),
                [](const auto &kv) { return kv.second; });
        if (ordinals != def.all_ordinals) {
            throw BindingException(def.name, "Enum definition is corrupted");
        }

        if (g_bound_enums.find(def.name) != g_bound_enums.cend()) {
            throw BindingException(def.name, "Enum with same name has already been bound");
        }

        if (g_bound_types.find(def.name) != g_bound_types.cend()) {
            throw BindingException(def.name, "Type with same name as enum has already been bound");
        }

        if (g_bound_global_fns.find(def.name) != g_bound_global_fns.cend()) {
            throw BindingException(def.name, "Global function with same name as enum has already been bound");
        }

        g_bound_enums.insert({ def.name, def });
        g_bound_enum_indices.insert({ def.type_index, def.name });
    }

    void apply_bindings_to_context(ScriptContext &context) {
        for (const auto &type : g_bound_types) {
            Logger::default_logger().debug("Binding type %s", type.second.name.c_str());

            context.pimpl->plugin->bind_type(context, type.second);
            Logger::default_logger().debug("Bound type %s", type.second.name.c_str());
        }

        for (const auto &type : g_bound_types) {
            Logger::default_logger().debug("Binding functions for type %s", type.second.name.c_str());

            for (const auto &type_fn : type.second.instance_functions) {
                Logger::default_logger().debug("Binding instance function %s::%s",
                        type.second.name.c_str(), type_fn.second.name.c_str());

                context.pimpl->plugin->bind_type_function(context, type.second, type_fn.second);

                Logger::default_logger().debug("Bound instance function %s::%s",
                        type.second.name.c_str(), type_fn.second.name.c_str());
            }

            for (const auto &type_fn : type.second.static_functions) {
                Logger::default_logger().debug("Binding static function %s::%s",
                        type.second.name.c_str(), type_fn.second.name.c_str());

                context.pimpl->plugin->bind_type_function(context, type.second, type_fn.second);

                Logger::default_logger().debug("Bound static function %s::%s",
                        type.second.name.c_str(), type_fn.second.name.c_str());
            }

            Logger::default_logger().debug("Bound %zu instance and %zu static functions for type %s",
                    type.second.instance_functions.size(), type.second.static_functions.size(),
                    type.second.name.c_str());
        }

        for (const auto &enum_def : g_bound_enums) {
            Logger::default_logger().debug("Binding enum %s", enum_def.second.name.c_str());

            context.pimpl->plugin->bind_enum(context, enum_def.second);

            Logger::default_logger().debug("Bound enum %s", enum_def.second.name.c_str());
        }

        for (const auto &fn : g_bound_global_fns) {
            Logger::default_logger().debug("Binding global function %s", fn.second.name.c_str());

            context.pimpl->plugin->bind_global_function(context, fn.second);

            Logger::default_logger().debug("Bound global function %s", fn.second.name.c_str());
        }
    }
}

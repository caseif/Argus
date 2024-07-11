/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/core/engine.hpp"

#include "argus/scripting/bind.hpp"
#include "argus/scripting/error.hpp"
#include "argus/scripting/types.hpp"

#include "internal/scripting/bind.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/util.hpp"
#include "internal/scripting/pimpl/script_context.hpp"
#include "argus/scripting/util.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>

namespace argus {
    [[nodiscard]] static Result<void, BindingError> _resolve_param(ObjectType &param_def, bool check_copyable = true) {
        if (param_def.type == IntegralType::Callback) {
            argus_assert(param_def.callback_type.has_value());
            auto &callback_type = *param_def.callback_type.value();
            for (auto &subparam : callback_type.params) {
                auto sub_res = _resolve_param(subparam);
                if (sub_res.is_err()) {
                    return sub_res;
                }
            }

            auto ret_res = _resolve_param(callback_type.return_type);
            if (ret_res.is_err()) {
                return ret_res;
            }

            return ok<void, BindingError>();
        } else if (param_def.type == IntegralType::Vector || param_def.type == IntegralType::VectorRef) {
            argus_assert(param_def.primary_type.has_value());
            auto el_res = _resolve_param(*param_def.primary_type.value(), false);
            if (el_res.is_err()) {
                return el_res;
            }

            return ok<void, BindingError>();
        } else if (param_def.type == IntegralType::Result) {
            argus_assert(param_def.primary_type.has_value());
            argus_assert(param_def.secondary_type.has_value());
            auto el_res = _resolve_param(*param_def.primary_type.value(), false)
                    .collate(_resolve_param(*param_def.secondary_type.value(), false));
            if (el_res.is_err()) {
                return el_res;
            }

            return ok<void, BindingError>();
        } else if (!is_bound_type(param_def.type)) {
            return ok<void, BindingError>();
        }

        argus_assert(param_def.type_index.has_value());
        argus_assert(!param_def.type_name.has_value());

        std::string type_name;
        if (param_def.type == IntegralType::Enum) {
            auto bound_enum_res = get_bound_enum(param_def.type_index.value());
            if (bound_enum_res.is_err()) {
                return err<void, BindingError>(param_def.type_index->name(),
                        "Failed to get enum while resolving function parameter");
            }
            type_name = bound_enum_res.unwrap().name;
        } else {
            auto bound_type_res = get_bound_type(param_def.type_index.value());
            if (bound_type_res.is_err()) {
                return err<void, BindingError>(param_def.type_index->name(),
                        "Failed to get type while resolving function parameter");
            }

            auto &bound_type = bound_type_res.unwrap();

            if (param_def.type == IntegralType::Struct) {
                if (check_copyable) {
                    if (bound_type.copy_ctor == nullptr) {
                        return err<void, BindingError>(bound_type.name,
                                "Class-typed parameter passed by value with type "
                                        + bound_type.name + " is not copy-constructible");
                    }

                    if (bound_type.move_ctor == nullptr) {
                        return err<void, BindingError>(bound_type.name,
                                "Class-typed parameter passed by value with type "
                                        + bound_type.name + " is not move-constructible");
                    }

                    if (bound_type.dtor == nullptr) {
                        return err<void, BindingError>(bound_type.name,
                                "Class-typed parameter passed by value with type "
                                        + bound_type.name + " is not destructible");
                    }
                }
            }

            type_name = bound_type.name;
        }

        param_def.type_name = type_name;

        return ok<void, BindingError>();
    }

    [[nodiscard]] static Result<void, BindingError> _resolve_field(ObjectType &field_def) {
        if (field_def.type == IntegralType::Vector || field_def.type == IntegralType::VectorRef) {
            argus_assert(field_def.primary_type.has_value());
            auto el_res = _resolve_field(*field_def.primary_type.value());
            if (el_res.is_err()) {
                return el_res;
            }

            return ok<void, BindingError>();
        } else if (!is_bound_type(field_def.type)) {
            return ok<void, BindingError>();
        }

        argus_assert(field_def.type_index.has_value());
        argus_assert(!field_def.type_name.has_value());

        std::string type_name;
        if (field_def.type == IntegralType::Enum) {
            auto bound_enum_res = get_bound_enum(field_def.type_index.value());
            if (bound_enum_res.is_err()) {
                return err<void, BindingError>(bound_enum_res.unwrap_err());
            }
            type_name = bound_enum_res.unwrap().name;
        } else {
            auto bound_type_res = get_bound_type(field_def.type_index.value());
            if (bound_type_res.is_err()) {
                return err<void, BindingError>(bound_type_res.unwrap_err());
            }

            auto &bound_type = bound_type_res.unwrap();

            field_def.is_refable = bound_type.is_refable;

            if (!bound_type.is_refable) {
                if (bound_type.copy_ctor == nullptr) {
                    return err<void, BindingError>(bound_type.name,
                            "Class-typed field with non-AutoCleanupable type " + bound_type.name
                                    + " is not copy-constructible");
                }

                if (bound_type.move_ctor == nullptr) {
                    return err<void, BindingError>(bound_type.name,
                            "Class-typed field with non-AutoCleanupable type " + bound_type.name
                                    + " is not move-constructible");
                }

                if (bound_type.dtor == nullptr) {
                    return err<void, BindingError>(bound_type.name,
                            "Class-typed field with non-AutoCleanupable type " + bound_type.name
                                    + " is not destructible");
                }
            }

            type_name = bound_type.name;
        }

        field_def.type_name = type_name;

        return ok<void, BindingError>();
    }

    [[nodiscard]] static Result<void, BindingError> _resolve_param_types(BoundFunctionDef &fn_def) {
        for (auto &param : fn_def.params) {
            auto param_res = _resolve_param(param);
            if (param_res.is_err()) {
                return param_res;
            }
        }

        auto ret_res = _resolve_param(fn_def.return_type);
        if (ret_res.is_err()) {
            return ret_res;
        }

        return ok<void, BindingError>();
    }

    Result<void, BindingError> resolve_parameter_types(BoundTypeDef &type_def) {
        for (auto &fn_kv : type_def.instance_functions) {
            auto &fn = fn_kv.second;

            auto params_res = _resolve_param_types(fn);

            if (params_res.is_err()) {
                return params_res.map_err<BindingError>([&type_def, &fn](const auto &err) {
                    auto qual_name = get_qualified_function_name(fn.type, type_def.name, fn.name);
                    return BindingError { qual_name, err.msg };
                });
            }
        }

        for (auto &fn_kv : type_def.extension_functions) {
            auto &fn = fn_kv.second;

            auto params_res = _resolve_param_types(fn);

            if (params_res.is_err()) {
                return params_res.map_err<BindingError>([&type_def, &fn](const auto &err) {
                    auto qual_name = get_qualified_function_name(fn.type, type_def.name, fn.name);
                    return BindingError { qual_name, err.msg };
                });
            }
        }

        for (auto &fn_kv : type_def.static_functions) {
            auto &fn = fn_kv.second;

            auto params_res = _resolve_param_types(fn);

            if (params_res.is_err()) {
                return params_res.map_err<BindingError>([&type_def, &fn](const auto &err) {
                    auto qual_name = get_qualified_function_name(fn.type, type_def.name, fn.name);
                    return BindingError { qual_name, err.msg };
                });
            }
        }

        for (auto &field_kv : type_def.fields) {
            auto &field = field_kv.second;

            auto field_res = _resolve_field(field.m_type);

            if (field_res.is_err()) {
                return field_res.map_err<BindingError>([&type_def, &field](const auto &err) {
                    auto qual_name = get_qualified_field_name(type_def.name, field.m_name);
                    return BindingError { qual_name, err.msg };
                });
            }
        }

        return ok<void, BindingError>();
    }

    Result<void, BindingError> resolve_parameter_types(BoundFunctionDef &fn_def) {
        return _resolve_param_types(fn_def).map_err<BindingError>([&fn_def](const auto &err) {
            return BindingError { fn_def.name, err.msg };
        });
    }

    template<typename T>
    [[nodiscard]] Result<T &, BindingError> _get_bound_type(const std::type_index type_index) {
        auto index_it = g_bound_type_indices.find(std::type_index(type_index));
        if (index_it == g_bound_type_indices.cend()) {
            crash("Type %s is not bound (check binding order and ensure bind_type"
                      " is called after creating type definition)", type_index.name());
        }
        auto type_it = g_bound_types.find(index_it->second);
        argus_assert(type_it != g_bound_types.cend());
        return ok<T &, BindingError>(const_cast<T &>(type_it->second));
    }

    template<typename T>
    Result<T &, BindingError> _get_bound_enum(std::type_index enum_type_index) {
        auto index_it = g_bound_enum_indices.find(std::type_index(enum_type_index));
        if (index_it == g_bound_enum_indices.cend()) {
            return err<T &, BindingError>(enum_type_index.name(),
                    "Enum is not bound (check binding order and ensure bind_type "
                    "is called after creating type definition)");
        }
        auto enum_it = g_bound_enums.find(index_it->second);
        argus_assert(enum_it != g_bound_enums.cend());
        return ok<T &, BindingError>(const_cast<T &>(enum_it->second));
    }

    Result<BoundTypeDef, BindingError> create_type_def(const std::string &name, size_t size, std::type_index type_index, bool is_refable,
            CopyCtorProxy copy_ctor,
            MoveCtorProxy move_ctor,
            DtorProxy dtor) {
        if (size == 0) {
            crash("Bound types cannot be zero-sized");
        }

        BoundTypeDef def {
                name,
                size,
                type_index,
                is_refable,
                copy_ctor,
                move_ctor,
                dtor,
                {},
                {},
                {},
                {}
        };
        return ok<BoundTypeDef, BindingError>(def);
    }

    Result<void, BindingError> bind_type(const BoundTypeDef &def) {
        if (g_bound_types.find(def.name) != g_bound_types.cend()) {
            return err<void, BindingError>(def.name, "Type with same name has already been bound");
        }

        if (g_bound_global_fns.find(def.name) != g_bound_global_fns.cend()) {
            return err<void, BindingError>(def.name,
                    "Global function with same name as type has already been bound");
        }

        if (g_bound_enums.find(def.name) != g_bound_enums.cend()) {
            return err<void, BindingError>(def.name, "Enum with same name as type has already been bound");
        }

        //TODO: perform validation on member functions

        std::vector<std::string> static_fn_names;
        std::transform(def.static_functions.cbegin(), def.static_functions.cend(),
                std::back_inserter(static_fn_names),
                [](const auto &fn_def) { return fn_def.second.name; });
        if (std::set(static_fn_names.cbegin(), static_fn_names.cend()).size() != static_fn_names.size()) {
            return err<void, BindingError>(def.name,
                    "Bound script type contains duplicate static function definitions");
        }

        std::vector<std::string> instance_fn_names;
        std::transform(def.instance_functions.cbegin(), def.instance_functions.cend(),
                std::back_inserter(instance_fn_names),
                [](const auto &fn_def) { return fn_def.second.name; });
        std::transform(def.extension_functions.cbegin(), def.extension_functions.cend(),
                std::back_inserter(instance_fn_names),
                [](const auto &fn_def) { return fn_def.second.name; });
        if (std::set(instance_fn_names.cbegin(), instance_fn_names.cend()).size() != instance_fn_names.size()) {
            return err<void, BindingError>(def.name,
                    "Bound script type contains duplicate instance/extension function definitions");
        }

        g_bound_types.insert({ def.name, def });
        g_bound_type_indices.insert({ def.type_index, def.name });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> bind_enum(const BoundEnumDef &def) {
        // check for consistency
        std::unordered_set<uint64_t> ordinals;
        ordinals.reserve(def.values.size());
        std::transform(def.values.cbegin(), def.values.cend(), std::inserter(ordinals, ordinals.end()),
                [](const auto &kv) { return kv.second; });
        if (ordinals != def.all_ordinals) {
            return err<void, BindingError>(def.name, "Enum definition is corrupted");
        }

        if (g_bound_enums.find(def.name) != g_bound_enums.cend()) {
            return err<void, BindingError>(def.name, "Enum with same name has already been bound");
        }

        if (g_bound_types.find(def.name) != g_bound_types.cend()) {
            return err<void, BindingError>(def.name, "Type with same name as enum has already been bound");
        }

        if (g_bound_global_fns.find(def.name) != g_bound_global_fns.cend()) {
            return err<void, BindingError>(def.name,
                    "Global function with same name as enum has already been bound");
        }

        g_bound_enums.insert({ def.name, def });
        g_bound_enum_indices.insert({ def.type_index, def.name });

        return ok<void, BindingError>();
    }

    Result<BoundEnumDef, BindingError> create_enum_def(const std::string &name, size_t width, std::type_index type_index) {
        return ok<BoundEnumDef, BindingError>(BoundEnumDef { name, width, type_index, {}, {} });
    }

    Result<void, BindingError> add_enum_value(BoundEnumDef &def, const std::string &name, uint64_t value) {
        if (def.values.find(name) != def.values.cend()) {
            return err<void, BindingError>(def.name + "::" + name, "Enum value with same name is already bound");
        }

        if (def.all_ordinals.find(value) != def.all_ordinals.cend()) {
            return err<void, BindingError>(def.name + "::" + name,
                    "Enum value with same ordinal is already bound");
        }

        def.values.insert({ name, value });
        def.all_ordinals.insert(value);

        return ok<void, BindingError>();
    }

    Result<void, BindingError> bind_enum_value(std::type_index enum_type, const std::string &name, uint64_t value) {
        auto enum_def = _get_bound_enum<BoundEnumDef>(enum_type);
        if (enum_def.is_err()) {
            return err<void, BindingError>(enum_def.unwrap_err());
        }
        return add_enum_value(enum_def.unwrap(), name, value);
    }

    Result<void, BindingError> bind_global_function(const BoundFunctionDef &def) {
        if (g_bound_global_fns.find(def.name) != g_bound_global_fns.cend()) {
            return err<void, BindingError>(def.name, "Global function with same name has already been bound");
        }

        if (g_bound_types.find(def.name) != g_bound_types.cend()) {
            return err<void, BindingError>(def.name,
                    "Type with same name as global function has already been bound");
        }

        if (g_bound_enums.find(def.name) != g_bound_enums.cend()) {
            return err<void, BindingError>(def.name,
                    "Enum with same name as global function has already been bound");
        }

        //TODO: perform validation including:
        //  - check that params types aren't garbage
        //  - check that param sizes match types where applicable
        //  - ensure params passed by value are copy-constructible

        g_bound_global_fns.insert({ def.name, def });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> add_member_instance_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (type_def.instance_functions.find(fn_def.name) != type_def.instance_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::MemberInstance, type_def.name, fn_def.name);
            return err<void, BindingError>(qual_name, "Instance function with same name is already bound");
        }

        type_def.instance_functions.insert({ fn_def.name, fn_def });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> bind_member_instance_function(std::type_index type_index, const BoundFunctionDef &fn_def) {
        auto type_def = _get_bound_type<BoundTypeDef>(type_index);
        if (type_def.is_err()) {
            return err<void, BindingError>(type_def.unwrap_err());
        }
        return add_member_instance_function(type_def.unwrap(), fn_def);
    }

    Result<void, BindingError> add_member_static_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (type_def.static_functions.find(fn_def.name) != type_def.static_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::MemberStatic, type_def.name, fn_def.name);
            return err<void, BindingError>(qual_name, "Static function with same name is already bound");
        }

        type_def.static_functions.insert({ fn_def.name, fn_def });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> bind_member_static_function(std::type_index type_index, const BoundFunctionDef &fn_def) {
        auto type_def = _get_bound_type<BoundTypeDef>(type_index);
        if (type_def.is_err()) {
            return err<void, BindingError>(type_def.unwrap_err());
        }
        return add_member_static_function(type_def.unwrap(), fn_def);
    }

    Result<void, BindingError> add_extension_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (fn_def.params.empty()
                || !(fn_def.params[0].type == IntegralType::Struct || fn_def.params[0].type == IntegralType::Pointer)
                || fn_def.params[0].type_index != type_def.type_index) {
            auto qual_name = get_qualified_function_name(FunctionType::Extension, type_def.name, fn_def.name);
            return err<void, BindingError>(qual_name,
                    "First parameter of extension function must match extended type");
        }

        if (type_def.extension_functions.find(fn_def.name) != type_def.extension_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::Extension, type_def.name, fn_def.name);
            return err<void, BindingError>(qual_name, "Extension function with same name is already bound");
        }

        if (type_def.instance_functions.find(fn_def.name) != type_def.instance_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::Extension, type_def.name, fn_def.name);
            return err<void, BindingError>(qual_name, "Instance function with same name is already bound");
        }

        type_def.extension_functions.insert({ fn_def.name, fn_def });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> bind_extension_function(std::type_index type_index, const BoundFunctionDef &fn_def) {
        auto type_def = _get_bound_type<BoundTypeDef>(type_index);
        if (type_def.is_err()) {
            return err<void, BindingError>(type_def.unwrap_err());
        }
        return add_extension_function(type_def.unwrap(), fn_def);
    }

    Result<void, BindingError> add_member_field(BoundTypeDef &type_def, const BoundFieldDef &field_def) {
        if (field_def.m_type.type == IntegralType::Callback) {
            auto qual_name = get_qualified_field_name(type_def.name, field_def.m_name);
            return err<void, BindingError>(qual_name, "Callback-typed fields are not supported");
        }

        if (type_def.fields.find(field_def.m_name) != type_def.fields.cend()) {
            auto qual_name = get_qualified_field_name(type_def.name, field_def.m_name);
            return err<void, BindingError>(qual_name, "Field with same name is already bound for type");
        }

        type_def.fields.insert({ field_def.m_name, field_def });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> bind_member_field(std::type_index type_index, const BoundFieldDef &field_def) {
        auto type_def = _get_bound_type<BoundTypeDef>(type_index);
        if (type_def.is_err()) {
            return err<void, BindingError>(type_def.unwrap_err());
        }
        return add_member_field(type_def.unwrap(), field_def);
    }

    Result<const BoundTypeDef &, BindingError> get_bound_type(const std::string &type_name) {
        auto it = g_bound_types.find(type_name);
        if (it == g_bound_types.cend()) {
            return err<const BoundTypeDef &, BindingError>(type_name,
                    "Type name is not bound (check binding order and ensure bind_type is called after "
                    "creating type definition)");
        }
        return ok<const BoundTypeDef &, BindingError>(it->second);
    }

    Result<const BoundTypeDef &, BindingError> get_bound_type(const std::type_info &type_info) {
        return get_bound_type(std::type_index(type_info));
    }

    Result<const BoundTypeDef &, BindingError> get_bound_type(std::type_index type_index) {
        return _get_bound_type<const BoundTypeDef>(type_index);
    }

    Result<const BoundEnumDef &, BindingError> get_bound_enum(const std::string &enum_name) {
        auto it = g_bound_enums.find(enum_name);
        if (it == g_bound_enums.cend()) {
            return err<const BoundEnumDef &, BindingError>(enum_name,
                    "Enum name is not bound (check binding order and ensure bind_enum "
                    "is called after creating enum definition)");
        }
        return ok<const BoundEnumDef &, BindingError>(it->second);
    }

    Result<const BoundEnumDef &, BindingError> get_bound_enum(const std::type_info &enum_type_info) {
        return get_bound_enum(std::type_index(enum_type_info));
    }

    Result<const BoundEnumDef &, BindingError> get_bound_enum(std::type_index enum_type_index) {
        return _get_bound_enum<const BoundEnumDef>(enum_type_index);
    }

    Result<void, BindingError> apply_bindings_to_context(ScriptContext &context) {
        for (const auto &[_, type] : g_bound_types) {
            Logger::default_logger().debug("Binding type %s", type.name.c_str());

            context.m_pimpl->plugin->bind_type(context, type);
            Logger::default_logger().debug("Bound type %s", type.name.c_str());
        }

        for (const auto &[_, type] : g_bound_types) {
            Logger::default_logger().debug("Binding functions for type %s", type.name.c_str());

            for (const auto &[_2, type_fn] : type.instance_functions) {
                Logger::default_logger().debug("Binding instance function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());

                context.m_pimpl->plugin->bind_type_function(context, type, type_fn);

                Logger::default_logger().debug("Bound instance function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());
            }

            for (const auto &[_2, type_fn] : type.extension_functions) {
                Logger::default_logger().debug("Binding extension function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());

                context.m_pimpl->plugin->bind_type_function(context, type, type_fn);

                Logger::default_logger().debug("Bound extension function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());
            }

            for (const auto &[_2, type_fn] : type.static_functions) {
                Logger::default_logger().debug("Binding static function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());

                context.m_pimpl->plugin->bind_type_function(context, type, type_fn);

                Logger::default_logger().debug("Bound static function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());
            }

            Logger::default_logger().debug("Bound %zu instance, %zu extension, and %zu static functions for type %s",
                    type.instance_functions.size(), type.extension_functions.size(),
                    type.static_functions.size(), type.name.c_str());
        }

        for (const auto &[_, type] : g_bound_types) {
            Logger::default_logger().debug("Binding fields for type %s", type.name.c_str());

            for (const auto &[_2, type_field] : type.fields) {
                Logger::default_logger().debug("Binding field %s::%s",
                        type.name.c_str(), type_field.m_name.c_str());

                context.m_pimpl->plugin->bind_type_field(context, type, type_field);

                Logger::default_logger().debug("Bound field %s::%s",
                        type.name.c_str(), type_field.m_name.c_str());
            }

            Logger::default_logger().debug("Bound %zu fields for type %s",
                    type.fields.size(), type.name.c_str());
        }

        for (const auto &[_, enum_def] : g_bound_enums) {
            Logger::default_logger().debug("Binding enum %s", enum_def.name.c_str());

            context.m_pimpl->plugin->bind_enum(context, enum_def);

            Logger::default_logger().debug("Bound enum %s", enum_def.name.c_str());
        }

        for (const auto &[_, fn] : g_bound_global_fns) {
            Logger::default_logger().debug("Binding global function %s", fn.name.c_str());

            context.m_pimpl->plugin->bind_global_function(context, fn);

            Logger::default_logger().debug("Bound global function %s", fn.name.c_str());
        }

        return ok<void, BindingError>();
    }
}

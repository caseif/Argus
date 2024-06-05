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

#include "argus/core/engine.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/exception.hpp"
#include "argus/scripting/types.hpp"
#include "argus/scripting/util.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <string>
#include <vector>

namespace argus {
    static const BoundFunctionDef &_get_native_function(FunctionType fn_type,
            const std::string &type_name, const std::string &fn_name) {
        switch (fn_type) {
            case MemberInstance:
            case MemberStatic:
            case Extension: {
                auto type_it = g_bound_types.find(type_name);
                if (type_it == g_bound_types.cend()) {
                    throw TypeNotBoundException(type_name);
                }

                const auto &fn_map = fn_type == MemberInstance
                        ? type_it->second.instance_functions
                        : fn_type == Extension
                                ? type_it->second.extension_functions
                                : type_it->second.static_functions;

                auto fn_it = fn_map.find(fn_name);
                if (fn_it == fn_map.cend()) {
                    throw SymbolNotBoundException(get_qualified_function_name(fn_type, type_name, fn_name));
                }
                return fn_it->second;
            }
            case Global: {
                auto it = g_bound_global_fns.find(fn_name);
                if (it == g_bound_global_fns.cend()) {
                    throw SymbolNotBoundException(fn_name);
                }
                return it->second;
            }
            default:
                crash("Unknown function type ordinal %d", fn_type);
        }
    }

    static const BoundFieldDef &_get_native_field(const std::string &type_name, const std::string &field_name) {
        auto type_it = g_bound_types.find(type_name);
        if (type_it == g_bound_types.cend()) {
            throw TypeNotBoundException(type_name);
        }

        const auto &field_map = type_it->second.fields;

        auto field_it = field_map.find(field_name);
        if (field_it == field_map.cend()) {
            throw SymbolNotBoundException(get_qualified_field_name(type_name, field_name));
        }
        return field_it->second;
    }

    const BoundFunctionDef &get_native_global_function(const std::string &name) {
        return _get_native_function(FunctionType::Global, "", name);
    }

    const BoundFunctionDef &get_native_member_instance_function(const std::string &type_name,
            const std::string &fn_name) {
        return _get_native_function(FunctionType::MemberInstance, type_name, fn_name);
    }

    const BoundFunctionDef &get_native_extension_function(const std::string &type_name,
            const std::string &fn_name) {
        return _get_native_function(FunctionType::Extension, type_name, fn_name);
    }

    const BoundFunctionDef &get_native_member_static_function(const std::string &type_name,
            const std::string &fn_name) {
        return _get_native_function(FunctionType::MemberStatic, type_name, fn_name);
    }

    const BoundFieldDef &get_native_member_field(const std::string &type_name, const std::string &field_name) {
        return _get_native_field(type_name, field_name);
    }

    ObjectWrapper invoke_native_function(const BoundFunctionDef &def, const std::vector<ObjectWrapper> &params) {
        auto expected_param_count = def.params.size();
        if (def.type == FunctionType::MemberInstance) {
            expected_param_count += 1;
        }

        if (params.size() < expected_param_count) {
            throw ReflectiveArgumentsException("Too few arguments provided");
        } else if (params.size() > expected_param_count) {
            throw ReflectiveArgumentsException("Too many arguments provided");
        }

        assert(params.size() == expected_param_count);

        return def.handle(params);
    }
}

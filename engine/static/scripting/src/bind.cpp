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

#include "internal/scripting/module_scripting.hpp"
#include "argus/scripting/util.hpp"

#include <set>
#include <string>
#include <unordered_set>

namespace argus {
    Result<BoundTypeDef, BindingError> create_type_def(const std::string &name, size_t size, const std::string &type_id,
            bool is_refable,
            CopyCtorProxy copy_ctor,
            MoveCtorProxy move_ctor,
            DtorProxy dtor) {
        if (size == 0) {
            crash("Bound types cannot be zero-sized");
        }

        BoundTypeDef def {
                name,
                size,
                type_id,
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

    Result<BoundEnumDef, BindingError> create_enum_def(const std::string &name, size_t width,
            const std::string &type_id) {
        return ok<BoundEnumDef, BindingError>(BoundEnumDef { name, width, type_id, {}, {} });
    }

    Result<void, BindingError> add_enum_value(BoundEnumDef &def, const std::string &name, int64_t value) {
        if (def.values.find(name) != def.values.cend()) {
            return err<void, BindingError>(BindingErrorType::DuplicateName, def.name + "::" + name,
                    "Enum value with same name is already bound");
        }

        if (def.all_ordinals.find(value) != def.all_ordinals.cend()) {
            return err<void, BindingError>(BindingErrorType::Other, def.name + "::" + name,
                    "Enum value with same ordinal is already bound");
        }

        def.values.insert({ name, value });
        def.all_ordinals.insert(value);

        return ok<void, BindingError>();
    }

    Result<void, BindingError> add_member_instance_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (type_def.instance_functions.find(fn_def.name) != type_def.instance_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::MemberInstance, type_def.name, fn_def.name);
            return err<void, BindingError>(BindingErrorType::DuplicateName, qual_name,
                    "Instance function with same name is already bound");
        }

        type_def.instance_functions.insert({ fn_def.name, fn_def });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> add_member_static_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (type_def.static_functions.find(fn_def.name) != type_def.static_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::MemberStatic, type_def.name, fn_def.name);
            return err<void, BindingError>(BindingErrorType::DuplicateName, qual_name,
                    "Static function with same name is already bound");
        }

        type_def.static_functions.insert({ fn_def.name, fn_def });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> add_extension_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (fn_def.params.empty()
                || !(fn_def.params[0].type == IntegralType::Struct || fn_def.params[0].type == IntegralType::Pointer)
                || fn_def.params[0].type_id != type_def.type_id) {
            auto qual_name = get_qualified_function_name(FunctionType::Extension, type_def.name, fn_def.name);
            return err<void, BindingError>(BindingErrorType::InvalidDefinition, qual_name,
                    "First parameter of extension function must match extended type");
        }

        if (type_def.extension_functions.find(fn_def.name) != type_def.extension_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::Extension, type_def.name, fn_def.name);
            return err<void, BindingError>(BindingErrorType::DuplicateName, qual_name,
                    "Extension function with same name is already bound");
        }

        if (type_def.instance_functions.find(fn_def.name) != type_def.instance_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::Extension, type_def.name, fn_def.name);
            return err<void, BindingError>(BindingErrorType::ConflictingName, qual_name,
                    "Instance function with same name is already bound");
        }

        type_def.extension_functions.insert({ fn_def.name, fn_def });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> add_member_field(BoundTypeDef &type_def, const BoundFieldDef &field_def) {
        if (field_def.m_type.type == IntegralType::Callback) {
            auto qual_name = get_qualified_field_name(type_def.name, field_def.m_name);
            return err<void, BindingError>(BindingErrorType::InvalidDefinition, qual_name,
                    "Callback-typed fields are not supported");
        }

        if (type_def.fields.find(field_def.m_name) != type_def.fields.cend()) {
            auto qual_name = get_qualified_field_name(type_def.name, field_def.m_name);
            return err<void, BindingError>(BindingErrorType::DuplicateName, qual_name,
                    "Field with same name is already bound for type");
        }

        type_def.fields.insert({ field_def.m_name, field_def });

        return ok<void, BindingError>();
    }
}

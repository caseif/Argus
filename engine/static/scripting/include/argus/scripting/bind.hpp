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

#pragma once

#include "argus/lowlevel/result.hpp"

#include "argus/scripting/error.hpp"
#include "argus/scripting/types.hpp"

#include <string>

namespace argus {

    [[nodiscard]] Result<BoundTypeDef, BindingError> create_type_def(const std::string &name, size_t size,
            const std::string &type_id, bool is_refable,
            CopyCtorProxy copy_ctor,
            MoveCtorProxy move_ctor,
            DtorProxy dtor);

    [[nodiscard]] Result<BoundEnumDef, BindingError> create_enum_def(const std::string &name, size_t width,
            const std::string &type_id);

    [[nodiscard]] Result<void, BindingError> add_enum_value(BoundEnumDef &def, const std::string &name, int64_t value);

    [[nodiscard]] Result<void, BindingError> add_member_instance_function(BoundTypeDef &type_def,
            const BoundFunctionDef &fn_def);

    [[nodiscard]] Result<void, BindingError> add_member_static_function(BoundTypeDef &type_def,
            const BoundFunctionDef &fn_def);

    [[nodiscard]] Result<void, BindingError> add_extension_function(BoundTypeDef &type_def,
            const BoundFunctionDef &fn_def);

    [[nodiscard]] Result<void, BindingError> add_member_field(BoundTypeDef &type_def, const BoundFieldDef &field_def);
}

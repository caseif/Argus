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
#include "argus/scripting/script_context.hpp"
#include "argus/scripting/types.hpp"

namespace argus {
    [[nodiscard]] Result<void, BindingError> resolve_parameter_types(BoundTypeDef &type_def);

    [[nodiscard]] Result<void, BindingError> resolve_parameter_types(BoundFunctionDef &fn_def);

    [[nodiscard]] Result<void, BindingError> apply_bindings_to_context(ScriptContext &context);
}

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

#include "argus/lowlevel/result.hpp"

#include "argus/core/engine.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/error.hpp"
#include "argus/scripting/types.hpp"
#include "argus/scripting/util.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <string>
#include <vector>

namespace argus {
    Result<ObjectWrapper, ReflectiveArgumentsError> invoke_native_function(const BoundFunctionDef &def,
            std::vector<ObjectWrapper> &params) {
        auto expected_param_count = def.params.size();
        if (def.type == FunctionType::MemberInstance) {
            expected_param_count += 1;
        }

        if (params.size() < expected_param_count) {
            return err<ObjectWrapper, ReflectiveArgumentsError>("Too few arguments provided");
        } else if (params.size() > expected_param_count) {
            return err<ObjectWrapper, ReflectiveArgumentsError>("Too many arguments provided");
        }

        argus_assert(params.size() == expected_param_count);

        return def.handle(params);
    }
}

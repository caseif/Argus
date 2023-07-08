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

#include "argus/scripting/util.hpp"

#include <cassert>

namespace argus {
    std::string get_qualified_function_name(FunctionType fn_type, const std::string &type_name,
            const std::string &fn_name) {
        switch (fn_type) {
            case FunctionType::Global:
                return fn_name;
            case FunctionType::MemberInstance:
                return type_name + "#" + fn_name;
            case FunctionType::MemberStatic:
                return type_name + "::" + fn_name;
            default:
                assert(false);
        }
    }
}

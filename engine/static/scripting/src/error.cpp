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

#include "argus/scripting/error.hpp"
#include "argus/scripting/types.hpp"

#include <string>
#include <type_traits>

namespace argus {
    std::string BindingError::to_string(void) const {
        return "BindingError { "
                "bound_name = \""
                + bound_name
                + "\", message = \""
                + msg
                + "\" }";
    }

    std::string ScriptLoadError::to_string(void) const {
        return "ScriptLoadError { "
               "resource_uid = \""
                + resource_uid
                + "\", message = \""
                + msg
                + "\" }";
    }

    ScriptInvocationError::ScriptInvocationError(std::string fn_name, std::string message):
        function_name(fn_name),
        msg(message) {
    }

    std::string ScriptInvocationError::to_string(void) const {
        return "ScriptInvocationError { "
                "function_name = \""
                + function_name
                + "\", message = \""
                + msg
                + "\" }";
    }

    ReflectiveArgumentsError::ReflectiveArgumentsError(std::string reason):
        reason(reason) {
    }

    std::string ReflectiveArgumentsError::to_string(void) const {
        return "ReflectiveArgumentsError { "
               "reason = \""
                + reason
                + "\" }";
    }

    SymbolNotBoundError::SymbolNotBoundError(SymbolType symbol_type, std::string symbol_name):
        symbol_type(symbol_type),
        symbol_name(symbol_name) {
    }

    std::string SymbolNotBoundError::to_string(void) const {
        return "SymbolNotBoundError { "
               "symbol_type = "
                + std::to_string(std::underlying_type_t<SymbolType>(symbol_type))
                + ", symbol_name = \""
                + symbol_name
                + "\" }";
    }
}

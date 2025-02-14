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

#include <string>

namespace argus {
    enum class BindingErrorType {
        DuplicateName,
        ConflictingName,
        InvalidDefinition,
        InvalidMembers,
        UnknownParent,
        Other,
    };

    struct BindingError {
        BindingErrorType type;
        std::string bound_name;
        std::string msg;

        std::string to_string(void) const;
    };

    struct ScriptLoadError {
        std::string resource_uid;
        std::string msg;

        std::string to_string(void) const;
    };

    struct ScriptInvocationError {
        std::string function_name;
        std::string msg;

        ScriptInvocationError(std::string fn_name, std::string message);

        std::string to_string(void) const;
    };

    struct ReflectiveArgumentsError {
        std::string reason;

        ReflectiveArgumentsError(std::string reason);

        std::string to_string(void) const;
    };

    enum class SymbolType {
        Type,
        Field,
        Function,
    };

    struct SymbolNotBoundError {
        SymbolType symbol_type;
        std::string symbol_name;

        SymbolNotBoundError(SymbolType symbol_type, std::string symbol_name);

        std::string to_string(void) const;
    };
}

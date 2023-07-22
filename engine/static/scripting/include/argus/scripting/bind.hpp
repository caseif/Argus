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

#pragma once

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/types.hpp"

#include <functional>
#include <string>

namespace argus {
    void bind_type(const BoundTypeDef &def);

    void bind_global_function(const BoundFunctionDef &def);

    void bind_enum(const BoundEnumDef &def);

    template <typename T>
    typename std::enable_if<std::is_class_v<T>, void>::type bind_type(const std::string &name) {
        static_assert(std::is_base_of_v<ScriptBindable, T>, "Bound types must derive from ScriptBindable");
        auto def = create_type_def<T>(name);
        bind_type(def);
    }

    template <typename FuncType>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>, void>::type
    bind_global_function(const std::string &name, FuncType fn) {
        auto def = create_global_function_def<FuncType>(name, fn);
        bind_global_function(def);
    }
}

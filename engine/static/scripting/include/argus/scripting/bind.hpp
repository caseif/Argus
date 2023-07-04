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

    template <typename T>
    typename std::enable_if<std::is_class_v<T>, BoundTypeDef>::type bind_type(const std::string &name) {
        auto def = create_type_def<T>(name);
        bind_type(def);
    }

    template <typename FuncType>
    typename std::enable_if<std::is_function_v<FuncType>, BoundFunctionDef>::type
    bind_global_function(const std::string &name, FuncType fn) {
        auto def = create_global_function_def<FuncType>(name, fn);
        bind_global_function(def);
    }

    //void register_script_type(BoundTypeDef type_def);

    //void register_script_global_function(BoundFunctionDef fn_def);

    /*template <typename T>
    int register_global_function(const std::string &name, std::function<T> fn) {
        UNUSED(fn);
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wold-style-cast"
        return register_global_function(name, asMETHOD(T, operator()));
        #pragma GCC diagnostic pop
    }*/
}

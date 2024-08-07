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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/extra_type_traits.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/misc.hpp"
#include "argus/lowlevel/result.hpp"

#include "argus/scripting/error.hpp"
#include "argus/scripting/object_type.hpp"
#include "argus/scripting/types.hpp"
#include "argus/scripting/wrapper.hpp"

#include <functional>
#include <string>
#include <tuple>
#include <typeinfo>
#include <utility>
#include <vector>

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace argus {
    // forward declarations
    struct BoundFunctionDef;
    struct BoundTypeDef;
    struct ObjectType;

    template<typename T>
    void _destroy_ref_wrapped_obj(T &item) {
        if constexpr (is_reference_wrapper_v<std::decay<T>>) {
            using U = remove_reference_wrapper_t<std::decay_t<T>>;
            if constexpr (!std::is_trivially_destructible_v<U> && !std::is_scalar_v<U>) {
                item.get().~U();
            }
        }
    }

    // proxy function which unwraps the given parameter list, forwards it to
    // the provided function, and directly returns the result to the caller
    template<typename FuncType,
            typename ReturnType = typename function_traits<FuncType>::return_type>
    Result<ReturnType, ReflectiveArgumentsError> invoke_function(FuncType fn, std::vector<ObjectWrapper> &params) {
        using ClassType = typename function_traits<FuncType>::class_type;
        using ArgsTuple = typename function_traits<FuncType>::argument_types_wrapped;

        auto expected_param_count = std::tuple_size<ArgsTuple>::value
                + (std::is_member_function_pointer_v<FuncType> ? 1 : 0);
        if (params.size() != expected_param_count) {
            return err<ReturnType, ReflectiveArgumentsError>("Wrong parameter count (expected "
                    + std::to_string(expected_param_count) + " , actual " + std::to_string(params.size()) + ")");
        }

        size_t params_off = std::is_member_function_pointer_v<FuncType> ? 1 : 0;
        ScratchAllocator scratch;
        auto args = make_tuple_from_params<ArgsTuple>(params, params_off,
                std::make_index_sequence<std::tuple_size_v<ArgsTuple>> {}, scratch);

        if constexpr (!std::is_void_v<ClassType>) {
            auto &instance_param = params.front();
            ClassType &instance = instance_param.get_value<ClassType &>();
            auto closure = [&](auto &&... args) -> ReturnType {
                if constexpr (!std::is_void_v<ReturnType>) {
                    ReturnType retval = (instance.*fn)(std::forward<decltype(args)>(args)...);
                    // destroy parameters if needed
                    (_destroy_ref_wrapped_obj(args), ...);
                    return retval;
                } else {
                    (instance.*fn)(std::forward<decltype(args)>(args)...);
                    (_destroy_ref_wrapped_obj(args), ...);
                }
            };

            if constexpr (!std::is_void_v<ReturnType>) {
                return ok<ReturnType, ReflectiveArgumentsError>(std::apply(closure, args));
            } else {
                std::apply(closure, args);
                return ok<void, ReflectiveArgumentsError>();
            }
        } else {
            //TODO: This fails statically before _make_tuple_from_params, which
            // is where the actually useful diagnostic messages are. Would be
            // nice to fix this at some point.
            if constexpr (std::tuple_size_v<ArgsTuple> == 1) {
                std::tuple_element_t<0, ArgsTuple> obj = std::get<0>(args);
                UNUSED(obj);
            }
            auto closure = [&](auto &&... args) -> ReturnType {
                if constexpr (!std::is_void_v<ReturnType>) {
                    ReturnType retval = fn(args...);
                    (_destroy_ref_wrapped_obj(args), ...);
                    return retval;
                } else {
                    fn(args...);
                    (_destroy_ref_wrapped_obj(args), ...);
                }
            };
            if constexpr (!std::is_void_v<ReturnType>) {
                return ok<ReturnType, ReflectiveArgumentsError>(std::apply(closure, args));
            } else {
                std::apply(closure, args);
                return ok<void, ReflectiveArgumentsError>();
            }
        }
    }

    [[nodiscard]] Result<const BoundFunctionDef &, SymbolNotBoundError> get_native_global_function(
            const std::string &name);

    [[nodiscard]] Result<const BoundFunctionDef &, SymbolNotBoundError> get_native_member_instance_function(
            const std::string &type_name, const std::string &fn_name);

    [[nodiscard]] Result<const BoundFunctionDef &, SymbolNotBoundError> get_native_extension_function(
            const std::string &type_name, const std::string &fn_name);

    [[nodiscard]] Result<const BoundFunctionDef &, SymbolNotBoundError> get_native_member_static_function(
            const std::string &type_name, const std::string &fn_name);

    [[nodiscard]] Result<const BoundFieldDef &, SymbolNotBoundError> get_native_member_field(
            const std::string &type_name, const std::string &field_name);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> invoke_native_function(const BoundFunctionDef &def,
            std::vector<ObjectWrapper> &params);
}

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

#include "argus/lowlevel/extra_type_traits.hpp"
#include "argus/lowlevel/misc.hpp"

#include "argus/scripting/types.hpp"

#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace argus {
    template<typename T, DataFlowDirection flow_dir,
            bool is_const = std::is_const_v<std::remove_reference_t<std::remove_pointer_t<T>>>>
    ObjectType create_object_type();

    // Second parameter implies that reference types must derive
    // from AutoCleanupable.
    // Function return values are passed directly to the script, and we only
    // allow scripts to assume ownership of references if the pointed-to type
    // type derives from AutoCleanupable so that the handle can be automatically
    // invalidated when the object is destroyed.
    template<typename T>
    constexpr ObjectType (*create_return_object_type)() = create_object_type<T, DataFlowDirection::ToScript>;

    template<typename T>
    constexpr ObjectType (*_create_callback_return_object_type)()
            = create_object_type<T, DataFlowDirection::FromScript>;

    template<typename Tuple, DataFlowDirection flow_dir, size_t... Is>
    static std::vector<ObjectType> _tuple_to_object_types_impl(std::index_sequence<Is...>) {
        return std::vector<ObjectType> {
                create_object_type<std::tuple_element_t<Is, Tuple>, flow_dir>()...
        };
    }

    template<typename Tuple, DataFlowDirection flow_dir>
    std::vector<ObjectType> tuple_to_object_types() {
        return _tuple_to_object_types_impl<Tuple, flow_dir>(
                std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>> {});
    }

    // this is only enabled for std::functions, not for function pointers
    template<typename F,
            typename ReturnType = typename function_traits<F>::return_type,
            typename Args = typename function_traits<F>::argument_types>
    static std::enable_if_t<!std::is_function_v<F>, ScriptCallbackType> _create_callback_type() {
        return ScriptCallbackType {
                // Second parameter implies that reference types must derive
                // from AutoCleanupable.
                // Callback params are passed directly to the script, and we
                // only allow scripts to assume ownership of references if the
                // pointed-to type derives from AutoCleanupable so that the
                // handle can be automatically invalidated when the object is
                // destroyed.
                tuple_to_object_types<Args, DataFlowDirection::ToScript>(),
                _create_callback_return_object_type<ReturnType>()
        };
    }

    template<typename T, DataFlowDirection flow_dir, bool is_const>
    ObjectType create_object_type() {
        static_assert(!std::is_rvalue_reference_v<T>, "Rvalue reference types are not supported");

        using B = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;
        if constexpr (std::is_void_v<T>) {
            return { IntegralType::Void, 0 };
        } else if constexpr (is_std_function_v<B>) {
            static_assert(is_std_function_v<T>,
                    "Callback reference/pointer params in bound function are not supported (pass by value instead)");
            return { IntegralType::Callback, sizeof(ProxiedNativeFunction), false, std::nullopt, std::nullopt,
                    std::make_unique<ScriptCallbackType>(_create_callback_type<B>()), std::nullopt };
        } else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::type_index>) {
            return { IntegralType::Type, sizeof(std::type_index), is_const };
        } else if constexpr (is_std_vector_v<std::remove_cv_t<T>>) {
            using E = typename B::value_type;
            return { IntegralType::Vector, sizeof(void *), is_const, typeid(std::remove_cv_t<T>),
                    std::nullopt, std::nullopt, create_object_type<E, flow_dir>() };
        } else if constexpr (is_std_vector_v<B>) {
            // REALLY big headache, better to just not support it
            static_assert(!(flow_dir == DataFlowDirection::FromScript
                            && !std::is_const_v<std::remove_reference_t<std::remove_pointer_t<T>>>),
                    "Non-const vector reference parameters are not supported");
            using E = typename B::value_type;
            return { IntegralType::VectorRef, sizeof(void *), is_const, typeid(B), std::nullopt, std::nullopt,
                    create_object_type<E, flow_dir, is_const>() };
            } else if constexpr (is_result_v<std::remove_cv_t<T>>) {
            static_assert(flow_dir == DataFlowDirection::ToScript,
                    "Result types may not be passed or returned from scripts");
            using V = typename result_traits<B>::value_type;
            using E = typename result_traits<B>::error_type;
            return { IntegralType::Result, sizeof(T), is_const, typeid(T), std::nullopt, std::nullopt,
                    create_object_type<V, flow_dir, is_const>(), create_object_type<E, flow_dir, is_const>() };
        } else if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>) {
            return { IntegralType::Boolean, sizeof(bool), is_const };
        } else if constexpr (std::is_integral_v<std::remove_cv_t<T>>) {
            return { IntegralType::Integer, sizeof(T), is_const };
        } else if constexpr (std::is_same_v<std::remove_cv_t<T>, float>) {
            return { IntegralType::Float, sizeof(float), is_const };
        } else if constexpr (std::is_same_v<std::remove_cv_t<T>, double>) {
            return { IntegralType::Float, sizeof(double), is_const };
        } else if constexpr ((std::is_pointer_v<T>
                && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>)
                || std::is_same_v<B, std::string>) {
            return { IntegralType::String, 0, is_const };
        } else if constexpr (std::is_reference_v<T> || std::is_pointer_v<std::remove_reference_t<T>>) {
            // too much of a headache to worry about
            static_assert(std::is_class_v<B>, "Non-class reference params in bound functions are not supported");
            // no real use case for these, might as well simplify our implementation
            static_assert(!is_result_v<B>, "Result references in bound functions are not supported");
            // References passed to scripts must be invalidated when the
            // underlying object is destroyed, which is only possible if the
            // type derives from AutoCleanupable.
            static_assert(!(flow_dir == DataFlowDirection::ToScript && !std::is_base_of_v<AutoCleanupable, B>),
                    "Reference types which flow from the engine to a script "
                    "(function return values and callback parameters) must derive from AutoCleanupable");
            return { IntegralType::Pointer, sizeof(void *), is_const, typeid(B) };
        } else if constexpr (std::is_enum_v<T>) {
            return { IntegralType::Enum, sizeof(std::underlying_type_t<T>), is_const, typeid(B) };
        } else {
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public copy constructor if passed by value");
            static_assert(std::is_copy_assignable_v<B>,
                    "Types in bound functions must have a public copy assignment operator if passed by value");
            static_assert(std::is_move_constructible_v<B>,
                    "Types in bound functions must have a public move constructor if passed by value");
            static_assert(std::is_move_assignable_v<B>,
                    "Types in bound functions must have a public move assignment operator if passed by value");
            static_assert(std::is_destructible_v<B>,
                    "Types in bound functions must have a public destructor if passed by value");
            return { IntegralType::Struct, sizeof(T), is_const, typeid(B) };
        }
    }
}

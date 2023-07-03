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

#include "argus/lowlevel/functional.hpp"

#include "argus/scripting/types.hpp"

#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <cstring>

namespace argus {
    // forward declarations
    struct BoundFunctionDef;
    struct BoundTypeDef;
    struct ObjectType;

    class InvocationException : public std::exception {
      private:
        const std::string msg;

      public:
        InvocationException(std::string msg) : msg(std::move(msg)) {
        }

        /**
         * \copydoc std::exception::what()
         *
         * \return The exception message.
         */
        [[nodiscard]] const char *what(void) const noexcept override {
            return msg.c_str();
        }
    };

    template <typename FuncType, typename... Args,
            typename ReturnType = typename function_traits<FuncType>::return_type>
    ReturnType invoke_function(FuncType fn, const std::vector<ObjectProxy> &params) {
        using ClassType = typename function_traits<FuncType>::class_type;
        using ArgsTuple = typename function_traits<FuncType>::argument_types;

        if (params.size() != std::tuple_size<ArgsTuple>::value
                + (std::is_member_function_pointer_v<FuncType> ? 1 : 0)) {
            throw InvocationException("Wrong parameter count");
        }

        ArgsTuple args;
        auto it = params.begin() + (std::is_member_function_pointer_v<FuncType> ? 0 : 1);
        std::apply([&](auto&... el) {
            (([&]() {
                if constexpr (std::is_same_v<std::decay_t<decltype(el)>, std::string>) {
                    el = std::string(*reinterpret_cast<const char *>((it++)->ptr));
                } else {
                    el = *reinterpret_cast<std::remove_pointer_t<std::remove_reference_t<decltype(el)>>*>((it++)->ptr);
                }
            })(), ...);
        }, args);

        if constexpr (!std::is_void_v<ClassType>) {
            ++it;
            ClassType *instance = reinterpret_cast<ClassType*>(std::get<0>(args));
            return std::apply([=](auto&&... args){ return (instance->*fn)(std::forward<decltype(args)>(args)...); },
                    args);
        } else {
            return std::apply(fn, args);
        }
    }

    template <typename FuncType, typename... Args>
    ProxiedFunction create_function_proxy(FuncType fn) {
        using ReturnType = typename function_traits<FuncType>::return_type;
        if constexpr (!std::is_void_v<ReturnType>) {
            return [fn] (std::vector<ObjectProxy> params) {
                ReturnType ret = invoke_function(fn, params);
                if constexpr (std::is_reference_v<ReturnType>) {
                    return ObjectProxy { &ret };
                } else if constexpr (std::is_pointer_v<ReturnType>) {
                    return ObjectProxy { ret };
                } else {
                    return ObjectProxy { copy_value(&ret, sizeof(ReturnType)) };
                }
            };
        } else {
            return invoke_function<FuncType, Args...>;
        }

    }

    void *copy_value(void *src, size_t size);

    void free_value(void *buf);

    ObjectProxy invoke_native_function_global(const std::string &name, const std::vector<ObjectProxy> &params);

    ObjectProxy invoke_native_function_instance(const std::string &name, const std::string &type_name,
            ObjectProxy instance, const std::vector<ObjectProxy> &params);

    ObjectProxy create_object_proxy(const ObjectType &type, void *ptr);

    ObjectProxy create_object_proxy(const ObjectType &type, const std::string &str);

    template <typename T>
    static ObjectType _create_object_type(void) {
        if constexpr (std::is_same_v<std::make_signed<T>, int8_t>()) {
            return { IntegralType::Integer, 1, "" };
        } else if constexpr (std::is_same_v<std::make_signed<std::remove_const<T>>, int16_t>()) {
            return { IntegralType::Integer, 2, "" };
        } else if constexpr (std::is_same_v<std::make_signed<std::remove_const<T>>, int32_t>()) {
            return { IntegralType::Integer, 4, "" };
        } else if constexpr (std::is_same_v<std::make_signed<std::remove_const<T>>, int64_t>()) {
            return { IntegralType::Integer, 8, "" };
        } else if constexpr (std::is_same_v<std::remove_const<T>, float>) {
            return { IntegralType::Float, 4, "" };
        } else if constexpr (std::is_same_v<std::remove_const<T>, double>) {
            return { IntegralType::Float, 8, "" };
        } else if constexpr (std::is_same_v<std::remove_const<T>, char*>
                             || std::is_same_v<std::remove_const<std::remove_reference<T>>, std::string>) {
            return { IntegralType::String, 0, "" };
        } else {
            return { IntegralType::Opaque, 0, "" };
        }
    }

    template <typename Tuple, size_t... Is>
    auto _tuple_to_vector_impl(std::index_sequence<Is...>) {
        return std::vector<ObjectType>{_create_object_type<std::tuple_element_t<Is, Tuple>>()...};
    }

    template <typename Tuple>
    auto _tuple_to_vector() {
        return _tuple_to_vector_impl(std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
    }

    template <typename T>
    typename std::enable_if<std::is_class_v<T>, BoundTypeDef>::type create_type_def(const std::string &name) {
        BoundTypeDef def{};
        def.name = name;
        def.size = sizeof(T);
    }

    template <typename FuncType, typename... Args>
    BoundFunctionDef create_function_def(const std::string &name, FuncType fn) {
        using ArgsTuple = typename function_traits<FuncType>::argument_types;
        using ReturnType = typename function_traits<FuncType>::return_type;

        BoundFunctionDef def{};
        def.name = name;
        def.handle = create_function_proxy(fn);
        def.params = _tuple_to_vector<ArgsTuple>();
        def.return_type = _create_object_type<ReturnType>();

        return def;
    }

    template <typename FuncType, typename... Args>
    typename std::enable_if<std::is_member_function_pointer_v<FuncType>, void>::type
    add_member_function(BoundTypeDef type_def, const std::string &name, FuncType fn) {
        auto fn_def = create_function_def(name, fn);
        type_def.instance_functions.push_back(fn_def);
    }

    template <typename FuncType, typename... Args>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>, void>::type
    add_static_function(BoundTypeDef type_def, const std::string &name, FuncType fn) {
        auto fn_def = create_function_def(name, fn);
        type_def.static_functions.push_back(fn_def);
    }
}

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
#include "argus/lowlevel/logging.hpp"

#include "argus/scripting/types.hpp"

#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <csignal>
#include <cstdio>
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

    template <typename T>
    static ObjectType _create_object_type(void) {
        if constexpr (std::is_void_v<T>) {
            return { IntegralType::Void, 0, "" };
        } if constexpr (std::is_integral_v<std::remove_const_t<T>>) {
            if constexpr (std::is_same_v<std::make_signed_t<std::remove_const_t<T>>, int8_t>) {
                return { IntegralType::Integer, 1, "" };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_const_t<T>>, int16_t>) {
                return { IntegralType::Integer, 2, "" };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_const_t<T>>, int32_t>) {
                return { IntegralType::Integer, 4, "" };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_const_t<T>>, int64_t>) {
                return { IntegralType::Integer, 8, "" };
            } else {
                Logger::default_logger().fatal("Unknown integer type");
            }
        } else if constexpr (std::is_same_v<std::remove_const_t<T>, float>) {
            return { IntegralType::Float, 4, "" };
        } else if constexpr (std::is_same_v<std::remove_const_t<T>, double>) {
            return { IntegralType::Float, 8, "" };
        } else if constexpr (std::is_same_v<std::remove_const_t<T>, char *>
                             || std::is_same_v<std::remove_const_t<T>, const char *>
                             || std::is_same_v<std::remove_const_t<std::remove_reference_t<T>>, std::string>) {
            return { IntegralType::String, 0, "" };
        } else {
            return { IntegralType::Opaque, 0, "" };
        }
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr, size_t size);

    template <typename FuncType, typename... Args,
            typename ReturnType = typename function_traits<FuncType>::return_type>
    ReturnType invoke_function(FuncType fn, const std::vector<ObjectWrapper> &params) {
        using ClassType = typename function_traits<FuncType>::class_type;
        using ArgsTuple = typename function_traits<FuncType>::argument_types;

        if (params.size() != std::tuple_size<ArgsTuple>::value
                + (std::is_member_function_pointer_v<FuncType> ? 1 : 0)) {
            throw InvocationException("Wrong parameter count");
        }

        ArgsTuple args;
        auto it = params.begin() + (std::is_member_function_pointer_v<FuncType> ? 1 : 0);
        std::apply([&](auto&... el) {
            (([&]() {
                auto param = *(it++);
                void *ptr = param.is_on_heap ? param.heap_ptr : param.value;

                if constexpr (std::is_same_v<std::decay_t<decltype(el)>, std::string>) {
                    el = std::string(reinterpret_cast<const char *>(ptr));
                } else if constexpr (std::is_pointer_v<std::remove_reference_t<decltype(el)>>) {
                    el = reinterpret_cast<std::remove_pointer_t<std::remove_reference_t<decltype(el)>> *>(
                            param.is_on_heap ? param.heap_ptr : param.stored_ptr);
                } else {
                    el = *reinterpret_cast<std::remove_reference_t<decltype(el)>*>(
                            param.is_on_heap ? param.heap_ptr : param.value);
                }
            })(), ...);
        }, args);

        if constexpr (!std::is_void_v<ClassType>) {
            ++it;
            auto instance_param = params.front();
            ClassType *instance = reinterpret_cast<ClassType*>(instance_param.is_on_heap
                    ? instance_param.heap_ptr
                    : instance_param.stored_ptr);
            return std::apply([=](auto&&... args){ return (instance->*fn)(std::forward<decltype(args)>(args)...); },
                    args);
        } else {
            return std::apply(fn, args);
        }
    }

    template <typename FuncType, typename... Args>
    ProxiedFunction create_function_wrapper(FuncType fn) {
        using ReturnType = typename function_traits<FuncType>::return_type;
        if constexpr (!std::is_void_v<ReturnType>) {
            return [fn] (const std::vector<ObjectWrapper> &params) {
                ReturnType ret = invoke_function(fn, params);

                ObjectWrapper wrapper{};
                auto ret_obj_type = _create_object_type<ReturnType>();

                if constexpr (std::is_reference_v<ReturnType>) {
                    wrapper.type = ret_obj_type;
                    wrapper.stored_ptr = &ret;
                } else if constexpr (std::is_pointer_v<ReturnType>) {
                    wrapper.type = ret_obj_type;
                    wrapper.stored_ptr = ret;
                } else {
                    wrapper = create_object_wrapper(wrapper.type, &ret, sizeof(ReturnType));
                }

                return wrapper;
            };
        } else {
            return [fn] (const std::vector<ObjectWrapper> &params) {
                invoke_function(fn, params);
                return ObjectWrapper {};
            };
        }
    }

    template <typename Tuple, size_t... Is>
    static auto _tuple_to_vector_impl(std::index_sequence<Is...>) {
        return std::vector<ObjectType>{_create_object_type<std::tuple_element_t<Is, Tuple>>()...};
    }

    template <typename Tuple>
    static auto _tuple_to_vector() {
        return _tuple_to_vector_impl<Tuple>(std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
    }

    template <typename FuncType, typename... Args>
    static BoundFunctionDef _create_function_def(const std::string &name, FuncType fn, FunctionType type) {
        using ArgsTuple = typename function_traits<FuncType>::argument_types;
        using ReturnType = typename function_traits<FuncType>::return_type;

        BoundFunctionDef def{};
        def.name = name;
        def.type = type;
        def.handle = create_function_wrapper(fn);
        def.params = _tuple_to_vector<ArgsTuple>();
        def.return_type = _create_object_type<ReturnType>();

        return def;
    }

    const BoundFunctionDef &get_native_global_function(const std::string &name);

    const BoundFunctionDef &get_native_member_instance_function(const std::string &type_name, const std::string &fn_name);

    const BoundFunctionDef &get_native_member_static_function(const std::string &type_name, const std::string &fn_name);

    ObjectWrapper invoke_native_function(const BoundFunctionDef &def, const std::vector<ObjectWrapper> &params);

    ObjectWrapper create_object_wrapper(const ObjectType &type, void *ptr);

    ObjectWrapper create_object_wrapper(const ObjectType &type, const std::string &str);

    void cleanup_object_wrapper(ObjectWrapper &wrapper);

    void cleanup_object_wrappers(std::vector<ObjectWrapper> &wrapper);

    template <typename T>
    typename std::enable_if<std::is_class_v<T>, BoundTypeDef>::type create_type_def(const std::string &name) {
        BoundTypeDef def{};
        def.name = name;
        def.size = sizeof(T);
        return def;
    }

    template <typename FuncType>
    BoundFunctionDef create_global_function_def(const std::string &name, FuncType fn) {
        return _create_function_def(name, fn, FunctionType::Global);
    }

    void add_member_instance_function(BoundTypeDef type_def, BoundFunctionDef fn_def);

    void add_member_static_function(BoundTypeDef type_def, BoundFunctionDef fn_def);

    template <typename FuncType>
    typename std::enable_if<std::is_member_function_pointer_v<FuncType>, void>::type
    add_member_instance_function(BoundTypeDef type_def, const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def(fn_name, fn, FunctionType::MemberInstance);
        add_member_instance_function(type_def, fn_def);
    }

    template <typename FuncType>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>, void>::type
    add_member_static_function(BoundTypeDef type_def, const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def(fn_name, fn, FunctionType::MemberStatic);
        add_member_static_function(type_def, fn_def);
    }
}

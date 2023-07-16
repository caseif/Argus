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

#include "argus/lowlevel/extra_type_traits.hpp"
#include "argus/lowlevel/logging.hpp"

#include "argus/scripting/exception.hpp"
#include "argus/scripting/types.hpp"

#include <functional>
#include <stdexcept>
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

    const BoundTypeDef &get_bound_type(const std::string &type_name);

    const BoundTypeDef &get_bound_type(const std::type_info &type_info);

    const BoundTypeDef &get_bound_type(const std::type_index &type_index);

    template <typename T>
    const BoundTypeDef &get_bound_type(void) {
        return get_bound_type(typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>));
    }

    const BoundEnumDef &get_bound_enum(const std::string &enum_name);

    const BoundEnumDef &get_bound_enum(const std::type_info &enum_type_info);

    const BoundEnumDef &get_bound_enum(const std::type_index &enum_type_index);

    template <typename T>
    const BoundEnumDef &get_bound_enum(void) {
        return get_bound_enum(typeid(std::remove_const_t<T>));
    }

    template <typename T>
    static ObjectType _create_object_type(void) {
        using B = std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>;
        if constexpr (std::is_void_v<T>) {
            return { IntegralType::Void, 0 };
        } else if constexpr (std::is_integral_v<std::remove_const_t<T>>) {
            if constexpr (std::is_same_v<std::make_signed_t<std::remove_const_t<T>>, int8_t>) {
                return { IntegralType::Integer, 1 };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_const_t<T>>, int16_t>) {
                return { IntegralType::Integer, 2 };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_const_t<T>>, int32_t>) {
                return { IntegralType::Integer, 4 };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_const_t<T>>, int64_t>) {
                return { IntegralType::Integer, 8 };
            } else {
                Logger::default_logger().fatal("Unknown integer type");
            }
        } else if constexpr (std::is_same_v<std::remove_const_t<T>, float>) {
            return { IntegralType::Float, 4 };
        } else if constexpr (std::is_same_v<std::remove_const_t<T>, double>) {
            return { IntegralType::Float, 8 };
        } else if constexpr (std::is_same_v<std::remove_const_t<T>, char *>
                             || std::is_same_v<B, std::string>) {
            return { IntegralType::String, 0 };
        } else if constexpr (std::is_reference_v<T> || std::is_pointer_v<std::remove_reference_t<T>>) {
            static_assert(std::is_class_v<B>, "Non-class reference params are not permitted");
            return { IntegralType::Pointer, sizeof(void *), typeid(std::remove_reference_t<std::remove_pointer_t<T>>) };
        } else if constexpr (std::is_enum_v<T>) {
            return { IntegralType::Enum, sizeof(std::underlying_type_t<T>),
                    typeid(std::remove_reference_t<std::remove_pointer_t<T>>) };
        } else {
            return { IntegralType::Struct, sizeof(T), typeid(std::remove_reference_t<std::remove_pointer_t<T>>) };
        }
    }

    template <typename T>
    static T _unwrap_param(ObjectWrapper &param, std::vector<std::string> &string_pool) {
        if constexpr (std::is_same_v<std::remove_const_t<remove_reference_wrapper_t<std::decay_t<T>>>, std::string>) {
            return string_pool.emplace_back(reinterpret_cast<const char *>(
                    param.is_on_heap ? param.heap_ptr : param.value));
        } else if constexpr (std::is_same_v<std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>>, std::string>) {
            return &string_pool.emplace_back(reinterpret_cast<const char *>(
                    param.is_on_heap ? param.heap_ptr : param.value));
        } else if constexpr (is_reference_wrapper_v<T>) {
            return *reinterpret_cast<std::remove_reference_t<remove_reference_wrapper_t<T>> *>(
                    param.is_on_heap ? param.heap_ptr : param.stored_ptr);
        } else if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
            return reinterpret_cast<std::remove_pointer_t<std::remove_reference_t<T>> *>(
                    param.is_on_heap
                        ? param.heap_ptr
                        : param.type.type == IntegralType::String
                            ? param.value
                            : param.stored_ptr);
        } else {
            return *reinterpret_cast<std::remove_reference_t<T>*>(
                    param.is_on_heap ? param.heap_ptr : param.value);
        }
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr, size_t size);

    template <typename ArgsTuple, size_t... Is>
    ArgsTuple _make_tuple_from_params(const std::vector<ObjectWrapper>::const_iterator &params_it,
            std::index_sequence<Is...>, std::vector<std::string> &string_pool) {
        return std::make_tuple(_unwrap_param<std::tuple_element_t<Is, ArgsTuple>>(
                const_cast<ObjectWrapper &>(*(params_it + Is)), string_pool)...);
    }

    template <typename FuncType,
            typename ReturnType = typename function_traits<FuncType>::return_type>
    ReturnType invoke_function(FuncType fn, const std::vector<ObjectWrapper> &params) {
        using ClassType = typename function_traits<FuncType>::class_type;
        using ArgsTuple = typename function_traits<FuncType>::argument_types_wrapped;

        auto expected_param_count = std::tuple_size<ArgsTuple>::value
                + (std::is_member_function_pointer_v<FuncType> ? 1 : 0);
        if (params.size() != expected_param_count) {
            throw InvocationException("Wrong parameter count (expected " + std::to_string(expected_param_count)
                    + " , actual " + std::to_string(params.size()) + ")");
        }

        auto it = params.begin() + (std::is_member_function_pointer_v<FuncType> ? 1 : 0);
        std::vector<std::string> string_pool;
        auto args = _make_tuple_from_params<ArgsTuple>(it, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{},
                string_pool);

        if constexpr (!std::is_void_v<ClassType>) {
            ++it;
            auto instance_param = params.front();
            ClassType *instance = reinterpret_cast<ClassType *>(instance_param.is_on_heap
                    ? instance_param.heap_ptr
                    : instance_param.stored_ptr);
            return std::apply([&](auto&&... args){ return (instance->*fn)(std::forward<decltype(args)>(args)...); },
                    args);
        } else {
            return std::apply(fn, args);
        }
    }

    template <typename FuncType>
    ProxiedFunction create_function_wrapper(FuncType fn) {
        using ReturnType = typename function_traits<FuncType>::return_type;
        if constexpr (!std::is_void_v<ReturnType>) {
            return [fn] (const std::vector<ObjectWrapper> &params) {
                ReturnType ret = invoke_function(fn, params);

                ObjectWrapper wrapper{};
                auto ret_obj_type = _create_object_type<ReturnType>();
                // _create_object_type is used to create function definitions so
                // it doesn't attempt to resolve the type name
                if (ret_obj_type.type == IntegralType::Pointer
                        || ret_obj_type.type == IntegralType::Struct) {
                    ret_obj_type.type_name = get_bound_type<ReturnType>().name;
                } else if (ret_obj_type.type == IntegralType::Enum) {
                    ret_obj_type.type_name = get_bound_enum<ReturnType>().name;
                }

                if constexpr (std::is_reference_v<ReturnType>) {
                    wrapper.type = ret_obj_type;
                    wrapper.stored_ptr = &ret;
                } else if constexpr (std::is_pointer_v<ReturnType>) {
                    wrapper.type = ret_obj_type;
                    wrapper.stored_ptr = ret;
                } else {
                    wrapper = create_object_wrapper(ret_obj_type, &ret, sizeof(ReturnType));
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
    static std::vector<ObjectType> _tuple_to_object_types_impl(std::index_sequence<Is...>) {
        return std::vector<ObjectType> { _create_object_type<std::tuple_element_t<Is, Tuple>>()... };
    }

    template <typename Tuple>
    static std::vector<ObjectType> _tuple_to_object_types() {
        return _tuple_to_object_types_impl<Tuple>(std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
    }

    template <typename FuncType, typename... Args>
    static BoundFunctionDef _create_function_def(const std::string &name, FuncType fn, FunctionType type) {
        using ArgsTuple = typename function_traits<FuncType>::argument_types;
        using ReturnType = typename function_traits<FuncType>::return_type;

        try {
            BoundFunctionDef def{};
            def.name = name;
            def.type = type;
            def.handle = create_function_wrapper(fn);
            def.params = _tuple_to_object_types<ArgsTuple>();
            def.return_type = _create_object_type<ReturnType>();

            return def;
        } catch (const std::exception &ex) {
            throw BindingException(name, ex.what());
        }
    }

    const BoundFunctionDef &get_native_global_function(const std::string &name);

    const BoundFunctionDef &get_native_member_instance_function(const std::string &type_name,
            const std::string &fn_name);

    const BoundFunctionDef &get_native_member_static_function(const std::string &type_name,
            const std::string &fn_name);

    ObjectWrapper invoke_native_function(const BoundFunctionDef &def, const std::vector<ObjectWrapper> &params);

    ObjectWrapper create_object_wrapper(const ObjectType &type, void *ptr);

    ObjectWrapper create_object_wrapper(const ObjectType &type, const std::string &str);

    void cleanup_object_wrapper(ObjectWrapper &wrapper);

    void cleanup_object_wrappers(std::vector<ObjectWrapper> &wrapper);

    BoundTypeDef create_type_def(const std::string &name, size_t size, std::type_index type_index);

    template <typename T>
    typename std::enable_if<std::is_class_v<T>, BoundTypeDef>::type create_type_def(const std::string &name) {
        return create_type_def(name, sizeof(T),
                std::type_index(typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>)));
    }

    template <typename FuncType>
    BoundFunctionDef create_global_function_def(const std::string &name, FuncType fn) {
        return _create_function_def(name, fn, FunctionType::Global);
    }

    void add_member_instance_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def);

    template <typename FuncType>
    typename std::enable_if<std::is_member_function_pointer_v<FuncType>, void>::type
    add_member_instance_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def(fn_name, fn, FunctionType::MemberInstance);
        add_member_instance_function(type_def, fn_def);
    }

    void add_member_static_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def);

    template <typename FuncType>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>, void>::type
    add_member_static_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def(fn_name, fn, FunctionType::MemberStatic);
        add_member_static_function(type_def, fn_def);
    }

    BoundEnumDef create_enum_def(const std::string &name, size_t width, std::type_index type_index);

    template <typename E>
    typename std::enable_if<std::is_enum_v<E>, BoundEnumDef>::type
    create_enum_def(const std::string &name) {
        return create_enum_def(name, sizeof(std::underlying_type_t<E>),
                std::type_index(typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<E>>>)));
    }

    void add_enum_value(BoundEnumDef &def, const std::string &name, uint64_t value);

    template <typename T>
    typename std::enable_if<std::is_enum_v<T>>::type
    add_enum_value(BoundEnumDef &def, const std::string &name, T value) {
        if (std::is_signed_v<std::underlying_type_t<T>>) {
            add_enum_value(def, name, *reinterpret_cast<uint64_t *>(int64_t(value)));
        } else {
            add_enum_value(def, name, uint64_t(value));
        }
    }
}

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
#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/result.hpp"

#include "argus/core/engine.hpp"

#include "argus/scripting/error.hpp"
#include "argus/scripting/types.hpp"

#include <algorithm>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace argus {
    struct BindingError;

    [[nodiscard]] Result<const BoundTypeDef &, BindingError> get_bound_type(const std::string &type_id);

    template<typename T>
    [[nodiscard]] Result<const BoundTypeDef &, BindingError> get_bound_type(void);

    [[nodiscard]] Result<const BoundEnumDef &, BindingError> get_bound_enum(const std::string &enum_type_id);

    template<typename T>
    [[nodiscard]] Result<const BoundEnumDef &, BindingError> get_bound_enum(void);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_object_wrapper(
            const ObjectType &type, const void *ptr);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_object_wrapper(
            const ObjectType &type, const void *ptr,
            size_t size);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_int_object_wrapper(
            const ObjectType &type, int64_t val);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_float_object_wrapper(
            const ObjectType &type, double val);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_bool_object_wrapper(
            const ObjectType &type, bool val);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_enum_object_wrapper(
            const ObjectType &type, int64_t ordinal);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_string_object_wrapper(
            const ObjectType &type,
            const std::string &str);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_callback_object_wrapper(
            const ObjectType &type,
            const ProxiedScriptCallback &fn);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_vector_object_wrapper(
            const ObjectType &type,
            const void *data, size_t count);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_vector_object_wrapper(
            const ObjectType &vec_type,
            const VectorWrapper &vec);

    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_vector_ref_object_wrapper(
            const ObjectType &vec_type,
            VectorWrapper vec);

    template<typename V, typename E = typename std::remove_cv_t<V>::value_type, bool is_heap>
    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> _create_vector_object_wrapper(
            const ObjectType &type, V &vec) {
        static_assert(!std::is_function_v<E> && !is_std_function_v<E>, "Vectors of callbacks are not supported");
        static_assert(!is_std_vector_v<E>, "Vectors of vectors are not supported");
        static_assert(!std::is_same_v<E, bool>, "Vectors of booleans are not supported");
        static_assert(!std::is_same_v<std::remove_cv_t<std::remove_reference_t<E>>, char *>,
                "Vectors of C-strings are not supported (use std::string instead)");
        argus_assert(type.primary_type.has_value());

        // ensure the vector reference will remain valid
        if (type.type == IntegralType::VectorRef && is_heap) {
            return create_vector_ref_object_wrapper(type,
                    VectorWrapper(const_cast<std::remove_cv_t<V> &>(vec), *type.primary_type.value()));
        } else {
            if (type.primary_type.value()->type != IntegralType::String) {
                argus_assert(type.primary_type.value()->size == sizeof(E));
            }

            ObjectType real_type = type;
            real_type.type = IntegralType::Vector;
            return create_vector_object_wrapper(real_type, reinterpret_cast<const void *>(vec.data()), vec.size());
        }
    }

    template<typename V, typename E = typename std::remove_cv_t<V>::value_type>
    [[nodiscard]] inline Result<ObjectWrapper, ReflectiveArgumentsError> create_vector_object_wrapper_from_heap(
            const ObjectType &type, V &vec) {
        return _create_vector_object_wrapper<V, E, true>(type, vec);
    }

    template<typename V, typename E = typename std::remove_cv_t<V>::value_type>
    [[nodiscard]] inline Result<ObjectWrapper, ReflectiveArgumentsError> create_vector_object_wrapper_from_stack(
            const ObjectType &type, V &vec) {
        return _create_vector_object_wrapper<V, E, false>(type, vec);
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_result_object_wrapper(const ObjectType &res_type,
            bool is_ok, const ObjectType &resolved_type, size_t resolved_size, void *resolved_ptr);

    template<typename T, typename E>
    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_result_object_wrapper(
            const ObjectType &res_type, Result<T, E> &result) {
        return create_result_object_wrapper(
                res_type,
                result.is_ok(),
                *(result.is_ok() ? res_type.primary_type : res_type.secondary_type).value(),
                result.is_ok() ? sizeof(T) : sizeof(E),
                result.is_ok()
                        ? reinterpret_cast<void *>(&result.unwrap())
                        : reinterpret_cast<void *>(&result.unwrap_err())
        );
    }

    template<typename T>
    [[nodiscard]] Result<ObjectWrapper, ReflectiveArgumentsError> create_auto_object_wrapper(
            const ObjectType &type, T val) {
        using B = std::remove_cv_t<remove_reference_wrapper_t<std::remove_reference_t<std::remove_pointer_t<T>>>>;

        // It's possible for a script to pass a vector literal to a bound
        // function which expects a reference, so we need to force scripting
        // plugins to detect this scenario and copy the vector to the heap since
        // otherwise we can't automatically determine where the passed vector
        // resides.
        // We could require that val _always_ be a reference to heap memory,
        // but that would force additional complexity in plugin code which is
        // undesirable.
        static_assert(!is_std_vector_v<std::remove_cv<std::remove_reference_t<std::remove_pointer_t<T>>>>,
                "Vector objects must be wrapped explicitly");

        // char* check must come first because it also passes the is_integral check
        if constexpr (std::is_same_v<std::remove_cv<T>, char *>
                || std::is_same_v<T, const char *>) {
            return create_string_object_wrapper(type, std::string(val));
        } else if constexpr (std::is_integral_v<B>) {
            return create_int_object_wrapper(type, int64_t(val));
        } else if constexpr (std::is_floating_point_v<B>) {
            return create_float_object_wrapper(type, double(val));
        } else if constexpr (std::is_same_v<B, bool>) {
            return create_bool_object_wrapper(type, val);
        } else if constexpr (std::is_same_v<B, std::string>) {
            return create_string_object_wrapper(type, val);
        } else if constexpr (std::is_same_v<B, ProxiedScriptCallback>) {
            return create_callback_object_wrapper(type, val);
        } else if constexpr (std::is_pointer_v<std::remove_cv_t<std::remove_reference_t<T>>>
                || std::is_reference_v<T>) {
            auto *ptr = const_cast<B *>(val);
            return create_object_wrapper(type, ptr);
        } else if constexpr (is_reference_wrapper_v<std::remove_cv_t<std::remove_reference_t<T>>>) {
            auto *ptr = &const_cast<B &>(val.get());
            return create_object_wrapper(type, &ptr);
        } else {
            return create_object_wrapper(type, &val);
        }
    }

    template<typename ArgsTuple, size_t... Is>
    [[nodiscard]] static Result<std::vector<ObjectWrapper>, ReflectiveArgumentsError> _make_params_from_tuple_impl(
            ArgsTuple &tuple, const std::vector<ObjectType>::const_iterator &types_it, std::index_sequence<Is...>) {
        std::vector<Result<ObjectWrapper, ReflectiveArgumentsError>> results;
        (results.emplace_back(create_auto_object_wrapper<std::tuple_element_t<Is, ArgsTuple>>(*(types_it + Is),
                std::get<Is>(tuple))), ...);

        auto err_it = std::find_if(results.cbegin(), results.cend(), ([](const auto &res) { return res.is_err(); }));
        if (err_it != results.cend()) {
            return err<std::vector<ObjectWrapper>, ReflectiveArgumentsError>(err_it->unwrap_err());
        }

        std::vector<ObjectWrapper> values;
        for (auto &res : results) {
            values.emplace_back(std::move(res.unwrap()));
        }
        return ok<std::vector<ObjectWrapper>, ReflectiveArgumentsError>(values);
    }

    template<typename ArgsTuple>
    [[nodiscard]] static Result<std::vector<ObjectWrapper>, ReflectiveArgumentsError> _make_params_from_tuple(
            ArgsTuple &tuple, const std::vector<ObjectType>::const_iterator &types_it) {
        return _make_params_from_tuple_impl(tuple, types_it, std::make_index_sequence<std::tuple_size_v<ArgsTuple>> {});
    }

    template<typename T>
    reference_wrapped_t<T> _wrap_single_reference_type(T &&value) {
        return std::forward<T>(value);
    }

    template<typename T>
    static T unwrap_param(ObjectWrapper &param, ScratchAllocator *scratch) {
        using B = std::remove_const_t<remove_reference_wrapper_t<T>>;

        argus_assert(param.is_initialized);

        if constexpr (is_std_function_v<B>) {
            argus_assert(param.type.type == IntegralType::Callback);
            argus_assert(param.type.callback_type.has_value());

            using ReturnType = typename function_traits<B>::return_type;
            using ArgsTuple = typename function_traits<B>::argument_types_wrapped;

            auto &proxied_fn = param.get_value<ProxiedScriptCallback>();
            auto fn_copy = std::make_shared<ProxiedScriptCallback>(proxied_fn);

            auto param_types = param.type.callback_type.value()->params;
            for (auto &subparam : param_types) {
                if (subparam.type == IntegralType::Pointer
                        || subparam.type == IntegralType::Struct) {
                    argus_assert(subparam.type_id.has_value());
                    subparam.type_name = get_bound_type(subparam.type_id.value())
                            .expect("Tried to unwrap callback param with unbound struct type").name;
                } else if (subparam.type == IntegralType::Enum) {
                    argus_assert(subparam.type_id.has_value());
                    subparam.type_name = get_bound_enum(subparam.type_id.value())
                            .expect("Tried to unwrap callback param with unbound enum type").name;
                }
            }

            auto ret_type = param.type.callback_type.value()->return_type;
            if (ret_type.type == IntegralType::Pointer
                    || ret_type.type == IntegralType::Struct) {
                ret_type.type_name = get_bound_type<ReturnType>()
                        .expect("Tried to unwrap callback return type with unbound struct type").name;
            } else if (ret_type.type == IntegralType::Enum) {
                ret_type.type_name = get_bound_enum<ReturnType>()
                        .expect("Tried to unwrap callback return type with unbound enum type").name;
            }

            return [fn_copy = std::move(fn_copy), param_types](auto &&... args) {
                ScratchAllocator scratch;

                ArgsTuple tuple = std::make_tuple(_wrap_single_reference_type(args)...);
                auto wrapped_params = _make_params_from_tuple<ArgsTuple>(tuple,
                        param_types.cbegin());

                if (wrapped_params.is_err()) {
                    return err<ReturnType, ScriptInvocationError>("(callback)", wrapped_params.unwrap_err().reason);
                }

                if constexpr (!std::is_void_v<ReturnType>) {
                    auto retval = (*fn_copy)(wrapped_params.unwrap());
                    if (retval.is_err()) {
                        return err<ReturnType, ScriptInvocationError>(retval.unwrap_err());
                    }
                    return unwrap_param<ReturnType>(retval.unwrap(), &scratch);
                } else {
                    UNUSED(scratch);
                    auto res = ((*fn_copy)(wrapped_params.unwrap()));
                    if (res.is_ok()) {
                        return ok<void, ScriptInvocationError>();
                    } else {
                        //TODO: handle this further down in the stack, ideally in the core module
                        crash("Error occurred while invoking script callback '%s': %s",
                                res.unwrap_err().function_name.c_str(),
                                res.unwrap_err().msg.c_str());
                        //return err<void, ScriptInvocationError>(res.unwrap_err());
                    }
                }
            };
        } else if constexpr (std::is_same_v<B, std::string>) {
            argus_assert(param.type.type == IntegralType::String);

            if constexpr (std::is_same_v<std::remove_cv_t<T>, std::string>) {
                if (scratch != nullptr) {
                    return scratch->construct<std::string>(reinterpret_cast<const char *>(param.get_ptr0()));
                } else {
                    std::string str = param.get_value<const char *>();
                    return str;
                }
            } else {
                argus_assert(scratch != nullptr);
                std::string &str = scratch->construct<std::string>(reinterpret_cast<const char *>(param.get_ptr0()));
                return str;
            }
        } else if constexpr (std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, std::string>) {
            argus_assert(param.type.type == IntegralType::String);
            argus_assert(scratch != nullptr);
            std::string *str = &scratch->construct<std::string>(reinterpret_cast<const char *>(param.get_ptr0()));
            return str;
        } else if constexpr (is_std_vector_v<std::remove_cv_t<remove_reference_wrapper_t<T>>>) {
            using E = typename std::remove_cv_t<remove_reference_wrapper_t<T>>::value_type;

            const auto &obj = param.get_value<VectorObject>();

            if (obj.get_object_type() == VectorObjectType::ArrayBlob) {
                const ArrayBlob &blob = reinterpret_cast<const ArrayBlob &>(obj);
                std::vector<E> vec;
                vec.reserve(blob.size());
                if constexpr (std::is_trivially_copyable_v<E>) {
                    vec.insert(vec.end(), blob.data(), reinterpret_cast<E *>(blob.data()) + blob.size());
                } else {
                    for (size_t i = 0; i < blob.size(); i++) {
                        vec.push_back(blob.at<E>(i));
                    }
                }

                if constexpr (is_reference_wrapper_v<T>) {
                    std::vector<E> *ptr = &scratch->construct<std::vector<E>>(vec.cbegin(), vec.cend());
                    return *ptr;
                } else {
                    return vec;
                }
            } else if (obj.get_object_type() == VectorObjectType::VectorWrapper) {
                const auto &wrapper = reinterpret_cast<const VectorWrapper &>(obj);
                return wrapper.get_underlying_vector<E>();
            } else {
                crash("Invalid vector object type magic %d",
                        std::underlying_type_t<VectorObjectType>(obj.get_object_type()));
            }
        } else if constexpr (is_reference_wrapper_v<T>) {
            argus_assert(param.type.type == IntegralType::Pointer);
            if constexpr (std::is_same_v<B, std::string>) {
                return std::ref(param.get_value<const char *>());
            } else {
                return param.get_value<std::remove_reference_t<remove_reference_wrapper_t<T>>>();
            }
        } else if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
            argus_assert(param.type.type == IntegralType::Pointer);
            if constexpr (std::is_same_v<B, std::string>) {
                return param.get_value<const char *>();
            } else {
                return param.get_value<std::remove_pointer_t<std::remove_reference_t<T>>>();
            }
        } else {
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public copy constructor if passed by value");
            static_assert(std::is_move_constructible_v<B>,
                    "Types in bound functions must have a public move constructor if passed by value");
            static_assert(std::is_destructible_v<B>,
                    "Types in bound functions must have a public destructor if passed by value");

            return *reinterpret_cast<const std::remove_reference_t<T> *>(
                    param.is_on_heap ? param.heap_ptr : param.value);
        }
    }

    template<typename ArgsTuple, size_t... Is>
    ArgsTuple make_tuple_from_params(std::vector<ObjectWrapper> &params, size_t params_off,
            std::index_sequence<Is...>, ScratchAllocator &scratch) {
        return std::make_tuple(unwrap_param<std::tuple_element_t<Is, ArgsTuple>>(
                params[Is + params_off], &scratch)...);
    }

    void copy_wrapped_object(const ObjectType &obj_type, void *dst, const void *src, size_t max_len);

    void move_wrapped_object(const ObjectType &obj_type, void *dst, void *src, size_t size);

    void destruct_wrapped_object(const ObjectType &obj_type, void *ptr);
}

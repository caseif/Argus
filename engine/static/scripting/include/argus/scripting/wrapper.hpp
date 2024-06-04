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

#include "argus/scripting/types.hpp"

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace argus {
    const BoundTypeDef &get_bound_type(std::type_index type_index);

    template<typename T>
    const BoundTypeDef &get_bound_type(void);

    const BoundEnumDef &get_bound_enum(std::type_index enum_type_index);

    template<typename T>
    const BoundEnumDef &get_bound_enum(void);

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr);

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr, size_t size);

    ObjectWrapper create_int_object_wrapper(const ObjectType &type, int64_t val);

    ObjectWrapper create_float_object_wrapper(const ObjectType &type, double val);

    ObjectWrapper create_bool_object_wrapper(const ObjectType &type, bool val);

    ObjectWrapper create_enum_object_wrapper(const ObjectType &type, int64_t ordinal);

    ObjectWrapper create_string_object_wrapper(const ObjectType &type, const std::string &str);

    ObjectWrapper create_callback_object_wrapper(const ObjectType &type, const ProxiedFunction &fn);

    ObjectWrapper create_vector_object_wrapper(const ObjectType &type, const void *data, size_t count);

    ObjectWrapper create_vector_object_wrapper(const ObjectType &vec_type, const VectorWrapper &vec);

    ObjectWrapper create_vector_ref_object_wrapper(const ObjectType &vec_type, VectorWrapper vec);

    template<typename V, typename E = typename std::remove_cv_t<V>::value_type, bool is_heap>
    ObjectWrapper _create_vector_object_wrapper(const ObjectType &type, V &vec) {
        static_assert(!std::is_function_v<E> && !is_std_function_v<E>, "Vectors of callbacks are not supported");
        static_assert(!is_std_vector_v<E>, "Vectors of vectors are not supported");
        static_assert(!std::is_same_v<E, bool>, "Vectors of booleans are not supported");
        static_assert(!std::is_same_v<std::remove_cv_t<std::remove_reference_t<E>>, char *>,
                "Vectors of C-strings are not supported (use std::string instead)");
        assert(type.element_type.has_value());

        // ensure the vector reference will remain valid
        if (type.type == IntegralType::VectorRef && is_heap) {
            return create_vector_ref_object_wrapper(type,
                    VectorWrapper(const_cast<std::remove_cv_t<V> &>(vec), *type.element_type.value()));
        } else {
            if (type.element_type.value()->type != IntegralType::String) {
                assert(type.element_type.value()->size == sizeof(E));
            }

            ObjectType real_type = type;
            real_type.type = IntegralType::Vector;
            return create_vector_object_wrapper(real_type, reinterpret_cast<const void *>(vec.data()), vec.size());
        }
    }

    template<typename V, typename E = typename std::remove_cv_t<V>::value_type>
    inline ObjectWrapper create_vector_object_wrapper_from_heap(const ObjectType &type, V &vec) {
        return _create_vector_object_wrapper<V, E, true>(type, vec);
    }

    template<typename V, typename E = typename std::remove_cv_t<V>::value_type>
    inline ObjectWrapper create_vector_object_wrapper_from_stack(const ObjectType &type, V &vec) {
        return _create_vector_object_wrapper<V, E, false>(type, vec);
    }

    template<typename T>
    ObjectWrapper create_auto_object_wrapper(const ObjectType &type, T val) {
        using B = std::remove_cv_t<remove_reference_wrapper_t<std::remove_reference_t<std::remove_pointer_t<T>>>>;

        // It's possible for a script to pass a vector literal to a bound
        // function which expects a reference, and without forcing scripting
        // plugins to detect this scenario and copy the vector to the heap,
        // we can't automatically determine where the passed vector resides.
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
        } else if constexpr (std::is_same_v<B, ProxiedFunction>) {
            return create_callback_object_wrapper(type, val);
        } else if constexpr (std::is_pointer_v<std::remove_cv_t<std::remove_reference_t<T>>>
                || std::is_reference_v<T>) {
            return create_object_wrapper(type, const_cast<B *>(val));
        } else if constexpr (is_reference_wrapper_v<std::remove_cv_t<std::remove_reference_t<T>>>) {
            return create_object_wrapper(type, &const_cast<B &>(val.get()));
        } else {
            return create_object_wrapper(type, &val);
        }
    }

    template<typename ArgsTuple, size_t... Is>
    static std::vector<ObjectWrapper> _make_params_from_tuple_impl(ArgsTuple &tuple,
            const std::vector<ObjectType>::const_iterator &types_it, std::index_sequence<Is...>) {
        std::vector<ObjectWrapper> result;
        (result.emplace_back(create_auto_object_wrapper<std::tuple_element_t<Is, ArgsTuple>>(*(types_it + Is),
                std::get<Is>(tuple))), ...);
        return result;
    }

    template<typename ArgsTuple>
    static std::vector<ObjectWrapper> _make_params_from_tuple(ArgsTuple &tuple,
            const std::vector<ObjectType>::const_iterator &types_it) {
        return _make_params_from_tuple_impl(tuple, types_it, std::make_index_sequence<std::tuple_size_v<ArgsTuple>> {});
    }

    template<typename T>
    reference_wrapped_t<T> _wrap_single_reference_type(T &&value) {
        return std::forward<T>(value);
    }

    template<typename T>
    static T unwrap_param(ObjectWrapper &param, ScratchAllocator *scratch) {
        using B = std::remove_const_t<remove_reference_wrapper_t<T>>;
        if constexpr (is_std_function_v<B>) {
            assert(param.type.type == IntegralType::Callback);
            assert(param.type.callback_type.has_value());

            using ReturnType = typename function_traits<B>::return_type;
            using ArgsTuple = typename function_traits<B>::argument_types_wrapped;

            auto proxied_fn = reinterpret_cast<ProxiedFunction *>(param.get_ptr());
            auto fn_copy = std::make_shared<ProxiedFunction>(*proxied_fn);

            auto param_types = param.type.callback_type.value()->params;
            for (auto &subparam : param_types) {
                if (subparam.type == IntegralType::Pointer
                        || subparam.type == IntegralType::Struct) {
                    assert(subparam.type_index.has_value());
                    subparam.type_name = get_bound_type(subparam.type_index.value()).name;
                } else if (subparam.type == IntegralType::Enum) {
                    assert(subparam.type_index.has_value());
                    subparam.type_name = get_bound_enum(subparam.type_index.value()).name;
                }
            }

            auto ret_type = param.type.callback_type.value()->return_type;
            if (ret_type.type == IntegralType::Pointer
                    || ret_type.type == IntegralType::Struct) {
                ret_type.type_name = get_bound_type<ReturnType>().name;
            } else if (ret_type.type == IntegralType::Enum) {
                ret_type.type_name = get_bound_enum<ReturnType>().name;
            }

            return [fn_copy = std::move(fn_copy), param_types](auto &&... args) {
                ScratchAllocator scratch;

                ArgsTuple tuple = std::make_tuple(_wrap_single_reference_type(args)...);
                std::vector<ObjectWrapper> wrapped_params = _make_params_from_tuple<ArgsTuple>(tuple,
                        param_types.cbegin());

                if constexpr (!std::is_void_v<ReturnType>) {
                    auto retval = (*fn_copy)(wrapped_params);
                    return unwrap_param<ReturnType>(retval, &scratch);
                } else {
                    UNUSED(scratch);
                    (*fn_copy)(wrapped_params);
                    return;
                }
            };
        } else if constexpr (std::is_same_v<B, std::string>) {
            assert(param.type.type == IntegralType::String);

            if constexpr (std::is_same_v<std::remove_cv_t<T>, std::string>) {
                if (scratch != nullptr) {
                    return scratch->construct<std::string>(reinterpret_cast<const char *>(
                            param.is_on_heap ? param.heap_ptr : param.value));
                } else {
                    return std::string(reinterpret_cast<const char *>(param.is_on_heap ? param.heap_ptr : param.value));
                }
            } else {
                assert(scratch != nullptr);
                return scratch->construct<std::string>(reinterpret_cast<const char *>(
                        param.is_on_heap ? param.heap_ptr : param.value));
            }
        } else if constexpr (std::is_same_v<std::remove_const_t<std::remove_pointer_t<T>>, std::string>) {
            assert(param.type.type == IntegralType::String);
            assert(scratch != nullptr);
            return &scratch->construct<std::string>(reinterpret_cast<const char *>(
                    param.is_on_heap ? param.heap_ptr : param.value));
        } else if constexpr (is_std_vector_v<std::remove_cv_t<remove_reference_wrapper_t<T>>>) {
            using E = typename std::remove_cv_t<remove_reference_wrapper_t<T>>::value_type;

            VectorObject *obj = reinterpret_cast<VectorObject *>(param.get_ptr());

            if (obj->get_object_type() == VectorObjectType::ArrayBlob) {
                ArrayBlob *blob = reinterpret_cast<ArrayBlob *>(obj);
                std::vector<E> vec;
                vec.reserve(blob->size());
                if constexpr (std::is_trivially_copyable_v<E>) {
                    vec.insert(vec.end(), blob->data(), reinterpret_cast<E *>(blob->data()) + blob->size());
                } else {
                    for (size_t i = 0; i < blob->size(); i++) {
                        vec.push_back(blob->at<E>(i));
                    }
                }

                if constexpr (is_reference_wrapper_v<T>) {
                    std::vector<E> *ptr = &scratch->construct<std::vector<E>>(vec.cbegin(), vec.cend());
                    return *ptr;
                } else {
                    return vec;
                }
            } else if (obj->get_object_type() == VectorObjectType::VectorWrapper) {
                VectorWrapper *wrapper = reinterpret_cast<VectorWrapper *>(obj);
                return wrapper->get_underlying_vector<E>();
            } else {
                throw std::runtime_error("Invalid vector object type magic");
            }
        } else if constexpr (is_reference_wrapper_v<T>) {
            assert(param.type.type == IntegralType::Pointer);
            return *reinterpret_cast<std::remove_reference_t<remove_reference_wrapper_t<T>> *>(
                    param.is_on_heap ? param.heap_ptr : param.stored_ptr);
        } else if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
            assert(param.type.type == IntegralType::Pointer);
            return reinterpret_cast<std::remove_pointer_t<std::remove_reference_t<T>> *>(
                    param.is_on_heap
                            ? param.heap_ptr
                            : param.type.type == IntegralType::String
                            ? param.value
                            : param.stored_ptr);
        } else {
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public copy constructor if passed by value");
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public move constructor if passed by value");
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public destructor if passed by value");

            return *reinterpret_cast<std::remove_reference_t<T> *>(param.is_on_heap ? param.heap_ptr : param.value);
        }
    }

    template<typename ArgsTuple, size_t... Is>
    ArgsTuple make_tuple_from_params(const std::vector<ObjectWrapper>::const_iterator &params_it,
            std::index_sequence<Is...>, ScratchAllocator &scratch) {
        return std::make_tuple(unwrap_param<std::tuple_element_t<Is, ArgsTuple>>(
                const_cast<ObjectWrapper &>(*(params_it + Is)), &scratch)...);
    }
}

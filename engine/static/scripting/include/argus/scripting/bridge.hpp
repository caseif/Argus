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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/macros.hpp"
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

    const BoundTypeDef &get_bound_type(std::type_index type_index);

    template <typename T>
    const BoundTypeDef &get_bound_type(void) {
        return get_bound_type(typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>));
    }

    const BoundEnumDef &get_bound_enum(const std::string &enum_name);

    const BoundEnumDef &get_bound_enum(const std::type_info &enum_type_info);

    const BoundEnumDef &get_bound_enum(std::type_index enum_type_index);

    template <typename T>
    const BoundEnumDef &get_bound_enum(void) {
        return get_bound_enum(typeid(std::remove_const_t<T>));
    }

    template <typename FuncType>
    ProxiedFunction create_function_wrapper(FuncType fn);

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr);

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr, size_t size);

    ObjectWrapper create_string_object_wrapper(const ObjectType &type, const std::string &str);

    ObjectWrapper create_callback_object_wrapper(const ObjectType &type, const ProxiedFunction &fn);

    template <typename T>
    ObjectWrapper create_auto_object_wrapper(const ObjectType &type, T val) {
        using B = std::remove_cv_t<remove_reference_wrapper_t<std::remove_reference_t<std::remove_pointer_t<T>>>>;
        if constexpr (std::is_same_v<B, std::string>) {
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

    template <typename T, bool check_script_bindable>
    static ObjectType _create_object_type();

    // Pass true as the second template parameter to forbid passing a reference
    // to a script if the type doesn't derive from ScriptBindable. We forbid it
    // because we would have no way of destroying the handle when the pointed-to
    // object is destroyed.
    template <typename T>
    constexpr ObjectType(*_create_return_object_type)()  = _create_object_type<T, true>;

    // Pass false as the second template parameter because we only care about
    // references being passed from the engine to a script, and not the other
    // way around.
    template <typename T>
    constexpr ObjectType(*_create_callback_return_object_type)()  = _create_object_type<T, false>;

    template <typename Tuple, bool check_script_bindable, size_t... Is>
    static std::vector<ObjectType> _tuple_to_object_types_impl(std::index_sequence<Is...>) {
        return std::vector<ObjectType> {
            _create_object_type<std::tuple_element_t<Is, Tuple>, check_script_bindable>()...
        };
    }

    template <typename Tuple, bool check_script_bindable>
    static std::vector<ObjectType> _tuple_to_object_types() {
        return _tuple_to_object_types_impl<Tuple, check_script_bindable>(
                std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
    }

    // this is only enabled for std::functions, not for function pointers
    template <typename F,
            typename ReturnType = typename function_traits<F>::return_type,
            typename Args = typename function_traits<F>::argument_types>
    static std::enable_if_t<!std::is_function_v<F>, ScriptCallbackType> _create_callback_type() {
        return ScriptCallbackType {
                // Pass true as the second template param to indicate that
                // reference types are only allowed if they derive from
                // ScriptBindable.
                // Callback params are passed directly to the script, and we
                // only allow scripts to assume ownership of references if the
                // pointed-to type derives from ScriptBindable so that the
                // handle can be properly invalidated if the object is
                // destroyed.
                _tuple_to_object_types<Args, true>(),
                _create_callback_return_object_type<ReturnType>()
        };
    }

    template <typename FuncType, typename... Args>
    static BoundFunctionDef _create_function_def(const std::string &name, FuncType fn, FunctionType type) {
        using ArgsTuple = typename function_traits<FuncType>::argument_types;
        using ReturnType = typename function_traits<FuncType>::return_type;
        bool is_const = function_traits<FuncType>::is_const::value;

        try {
            BoundFunctionDef def{};
            def.name = name;
            def.type = type;
            def.is_const = is_const;
            def.handle = create_function_wrapper(fn);
            // pass false as the second template param because we only care
            // about ScriptBindable for objects being passed from the engine to
            // a script, and not the other way around
            def.params = _tuple_to_object_types<ArgsTuple, false>();
            def.return_type = _create_return_object_type<ReturnType>();

            return def;
        } catch (const std::exception &ex) {
            throw BindingException(name, ex.what());
        }
    }

    template <typename T, bool check_script_bindable>
    static ObjectType _create_object_type() {
        using B = std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>;
        if constexpr (std::is_void_v<T>) {
            return { IntegralType::Void, 0 };
        } else if constexpr (is_std_function_v<B>) {
            static_assert(is_std_function_v<T>, "Callback reference/pointer params in bound function are not"
                    "permitted (pass by value instead)");
            return { IntegralType::Callback, sizeof(ProxiedFunction), false, {}, {},
                     std::make_shared<ScriptCallbackType>(_create_callback_type<B>()) };
        } else if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::type_index>) {
            return { IntegralType::Type, sizeof(std::type_index) };
        } else if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>) {
            return { IntegralType::Boolean, sizeof(bool) };
        } else if constexpr (std::is_integral_v<std::remove_cv_t<T>>) {
            if constexpr (std::is_same_v<std::make_signed_t<std::remove_cv_t<T>>, int8_t>) {
                return { IntegralType::Integer, sizeof(int8_t) };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_cv_t<T>>, int16_t>) {
                return { IntegralType::Integer, sizeof(int16_t) };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_cv_t<T>>, int32_t>) {
                return { IntegralType::Integer, sizeof(int32_t) };
            } else if constexpr (std::is_same_v<std::make_signed_t<std::remove_cv_t<T>>, int64_t>) {
                return { IntegralType::Integer, sizeof(int64_t) };
            } else {
                Logger::default_logger().fatal("Unknown integer type");
            }
        } else if constexpr (std::is_same_v<std::remove_cv_t<T>, float>) {
            return { IntegralType::Float, sizeof(float) };
        } else if constexpr (std::is_same_v<std::remove_cv_t<T>, double>) {
            return { IntegralType::Float, sizeof(double) };
        } else if constexpr ((std::is_pointer_v<T>
                    && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>)
                || std::is_same_v<B, std::string>) {
            return { IntegralType::String, 0 };
        } else if constexpr (std::is_reference_v<T> || std::is_pointer_v<std::remove_reference_t<T>>) {
            static_assert(std::is_class_v<B>, "Non-class reference params in bound functions are not permitted");
            // we only allow scripts to receive references to types
            static_assert(!(check_script_bindable && !std::is_base_of_v<ScriptBindable, B>),
                    "Bound functions may not return a reference to a type which does not derive from ScriptBindable");
            return { IntegralType::Pointer, sizeof(void *),
                    std::is_const_v<std::remove_pointer_t<std::remove_reference_t<T>>>,
                    typeid(std::remove_reference_t<std::remove_pointer_t<T>>) };
        } else if constexpr (std::is_enum_v<T>) {
            return { IntegralType::Enum, sizeof(std::underlying_type_t<T>), false,
                    typeid(std::remove_reference_t<std::remove_pointer_t<T>>) };
        } else {
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public copy constructor if passed by value");
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public move constructor if passed by value");
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public destructor if passed by value");
            return { IntegralType::Struct, sizeof(T), false,
                    typeid(std::remove_reference_t<std::remove_pointer_t<T>>) };
        }
    }

    template <typename ArgsTuple, size_t... Is>
    static std::vector<ObjectWrapper> _make_params_from_tuple_impl(ArgsTuple &tuple,
            const std::vector<ObjectType>::const_iterator &types_it, std::index_sequence<Is...>) {
        std::vector<ObjectWrapper> result;
        (result.emplace_back(create_auto_object_wrapper<std::tuple_element_t<Is, ArgsTuple>>(*(types_it + Is), std::get<Is>(tuple))), ...);
        return result;
    }

    template <typename ArgsTuple>
    static std::vector<ObjectWrapper> _make_params_from_tuple(ArgsTuple &tuple,
            const std::vector<ObjectType>::const_iterator &types_it) {
        return _make_params_from_tuple_impl(tuple, types_it, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
    }

    template <typename T>
    reference_wrapped_t<T> _wrap_single_reference_type(T &&value) {
        return std::forward<T>(value);
    }

    template <typename T>
    static T _unwrap_param(ObjectWrapper &param, std::vector<std::string> *string_pool) {
        using B = std::remove_const_t<remove_reference_wrapper_t<std::decay_t<T>>>;
        if constexpr (is_std_function_v<B>) {
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
                std::vector<std::string> string_pool;

                ArgsTuple tuple = std::make_tuple(_wrap_single_reference_type(args)...);
                std::vector<ObjectWrapper> wrapped_params = _make_params_from_tuple<ArgsTuple>(tuple,
                        param_types.cbegin());

                if constexpr (!std::is_void_v<ReturnType>) {
                    auto retval = (*fn_copy)(wrapped_params);
                    return _unwrap_param<ReturnType>(retval, &string_pool);
                } else {
                    UNUSED(string_pool);
                    (*fn_copy)(wrapped_params);
                    return;
                }
            };
        } else if constexpr (std::is_same_v<B, std::string>) {
            if constexpr (std::is_same_v<std::remove_cv_t<T>, std::string>) {
                if (string_pool != nullptr) {
                    return string_pool->emplace_back(reinterpret_cast<const char *>(
                            param.is_on_heap ? param.heap_ptr : param.value));
                } else {
                    return std::string(reinterpret_cast<const char *>(param.is_on_heap ? param.heap_ptr : param.value));
                }
            } else {
                assert(string_pool != nullptr);
                return string_pool->emplace_back(reinterpret_cast<const char *>(
                        param.is_on_heap ? param.heap_ptr : param.value));
            }
        } else if constexpr (std::is_same_v<std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>>, std::string>) {
            assert(string_pool != nullptr);
            return &string_pool->emplace_back(reinterpret_cast<const char *>(
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
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public copy constructor if passed by value");
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public move constructor if passed by value");
            static_assert(std::is_copy_constructible_v<B>,
                    "Types in bound functions must have a public destructor if passed by value");

            return *reinterpret_cast<std::remove_reference_t<T>*>(
                    param.is_on_heap ? param.heap_ptr : param.value);
        }
    }

    template <typename ArgsTuple, size_t... Is>
    ArgsTuple _make_tuple_from_params(const std::vector<ObjectWrapper>::const_iterator &params_it,
            std::index_sequence<Is...>, std::vector<std::string> &string_pool) {
        return std::make_tuple(_unwrap_param<std::tuple_element_t<Is, ArgsTuple>>(
                const_cast<ObjectWrapper &>(*(params_it + Is)), &string_pool)...);
    }

    template <typename FieldType, typename ClassType>
    static BoundFieldDef _create_field_def(const std::string &name, FieldType(ClassType::* field)) {
        // treat C-string fields as const because there's no good way to manage their memory
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<FieldType>>
                || std::is_same_v<std::remove_cv_t<std::decay_t<FieldType>>, char *>;

        try {
            auto type = _create_object_type<FieldType, true>();
            type.is_const = is_const;

            BoundFieldDef def{};
            def.m_name = name;
            def.m_type = type;
            def.m_get_const_proxy = [field, type](const ObjectWrapper &inst, const ObjectType &field_type) {
                assert(field_type.type == type.type);
                assert(field_type.size == type.size);
                assert(field_type.type_index == type.type_index);
                assert(field_type.is_const == type.is_const);
                assert(field_type.callback_type == type.callback_type);

                const ClassType *instance = reinterpret_cast<const ClassType *>(inst.is_on_heap
                        ? inst.heap_ptr
                        : inst.stored_ptr);

                return create_object_wrapper(field_type, reinterpret_cast<const void *>(&(instance->*field)));
            };

            def.m_get_mut_proxy = [field](ObjectWrapper &inst, const ObjectType &field_type) {
                ClassType *instance = reinterpret_cast<ClassType *>(inst.is_on_heap
                        ? inst.heap_ptr
                        : inst.stored_ptr);

                auto real_type = field_type;
                if (field_type.type == IntegralType::Struct) {
                    real_type.type = IntegralType::Pointer;
                    real_type.size = sizeof(void *);
                }

                return create_auto_object_wrapper(real_type, instance->*field);
            };

            if constexpr (!is_const) {
                def.m_assign_proxy = [field](ObjectWrapper &inst, ObjectWrapper &val) {
                    ClassType *instance = reinterpret_cast<ClassType *>(inst.is_on_heap
                            ? inst.heap_ptr
                            : inst.stored_ptr);

                    instance->*field = _unwrap_param<FieldType>(val, nullptr);
                };
            }

            return def;
        } catch (const std::exception &ex) {
            throw BindingException(name, ex.what());
        }
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
            auto &instance_param = params.front();
            ClassType *instance = reinterpret_cast<ClassType *>(instance_param.is_on_heap
                    ? instance_param.heap_ptr
                    : instance_param.stored_ptr);
            return std::apply([&](auto&&... args) -> ReturnType {
                return (instance->*fn)(std::forward<decltype(args)>(args)...);
            }, args);
        } else {
            //TODO: This fails statically before _make_tuple_from_params, which
            // is where the actually useful diagnostic messages are. Would be
            // nice to fix this at some point.
            return std::apply(fn, args);
        }
    }

    template <typename FuncType>
    ProxiedFunction create_function_wrapper(FuncType fn) {
        using ReturnType = typename function_traits<FuncType>::return_type;
        if constexpr (!std::is_void_v<ReturnType>) {
            return [fn] (const std::vector<ObjectWrapper> &params) {
                ReturnType ret = invoke_function(fn, params);

                auto ret_obj_type = _create_return_object_type<ReturnType>();
                size_t ret_obj_size;
                if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<
                        std::remove_pointer_t<ReturnType>>>, std::string>
                        || std::is_same_v<std::remove_cv_t<std::remove_reference_t<
                                std::remove_pointer_t<ReturnType>>>, char>) {
                    if constexpr (std::is_reference_v<ReturnType>) {
                        static_assert(std::is_same_v<std::remove_cv_t<std::remove_reference_t<ReturnType>>,
                                              std::string>,
                                      "Returned string reference from bound function must be direct reference");

                        ret_obj_size = ret.length();
                    } else if constexpr (std::is_pointer_v<ReturnType>) {
                        using B = std::remove_cv_t<std::remove_pointer_t<ReturnType>>;
                        static_assert(std::is_same_v<B, std::string> || std::is_same_v<B, char>,
                                      "Returned string pointer from bound function must be direct pointer to "
                                      "std::string or char array");

                        if constexpr (std::is_same_v<B, std::string>) {
                            ret_obj_size = ret->length();
                        } else if constexpr (std::is_same_v<B, char>) {
                            ret_obj_size = strlen(ret);
                        } else {
                            // can't use static_assert because the enclosing block is checking runtime info
                            assert(false);
                        }
                    } else if constexpr (std::is_same_v<std::remove_cv_t<ReturnType>, std::string>) {
                        ret_obj_size = ret.length();
                    } else {
                        // can't use static_assert because the enclosing block is checking runtime info
                        assert(false);
                    }
                } else {
                    ret_obj_size = sizeof(ReturnType);
                }

                // _create_object_type is used to create function definitions
                // too so it doesn't attempt to resolve the type name
                if (ret_obj_type.type == IntegralType::Pointer
                        || ret_obj_type.type == IntegralType::Struct) {
                    ret_obj_type.type_name = get_bound_type<ReturnType>().name;
                } else if (ret_obj_type.type == IntegralType::Enum) {
                    ret_obj_type.type_name = get_bound_enum<ReturnType>().name;
                }

                ObjectWrapper wrapper(ret_obj_type, ret_obj_size);
                if constexpr (std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<ReturnType>>>, std::string>) {
                    return create_object_wrapper(ret_obj_type, ret.c_str(), ret.size());
                } else if constexpr (std::is_pointer_v<std::remove_cv_t<std::remove_reference_t<ReturnType>>>
                        && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<
                                std::remove_reference_t<ReturnType>>>, char>) {
                    return create_object_wrapper(ret_obj_type, ret, strlen(ret));
                } else if constexpr (std::is_reference_v<ReturnType>) {
                    wrapper.stored_ptr = const_cast<std::remove_const_t<std::remove_reference_t<ReturnType>> *>(&ret);
                    return wrapper;
                } else if constexpr (std::is_pointer_v<ReturnType>) {
                    wrapper.stored_ptr = const_cast<std::remove_const_t<std::remove_pointer_t<ReturnType>> *>(ret);
                    return wrapper;
                } else {
                    return create_object_wrapper(ret_obj_type, &ret, sizeof(ReturnType));
                }
            };
        } else {
            return [fn] (const std::vector<ObjectWrapper> &params) {
                invoke_function(fn, params);
                auto type = _create_return_object_type<void>();
                return ObjectWrapper(type, 0);
            };
        }
    }

    const BoundFunctionDef &get_native_global_function(const std::string &name);

    const BoundFunctionDef &get_native_member_instance_function(const std::string &type_name,
            const std::string &fn_name);

    const BoundFunctionDef &get_native_member_static_function(const std::string &type_name,
            const std::string &fn_name);

    const BoundFieldDef &get_native_member_field(const std::string &type_name, const std::string &field_name);

    ObjectWrapper invoke_native_function(const BoundFunctionDef &def, const std::vector<ObjectWrapper> &params);

    BoundTypeDef create_type_def(const std::string &name, size_t size, std::type_index type_index,
            std::optional<std::function<void(void *dst, const void *src)>> copy_ctor,
            std::optional<std::function<void(void *dst, void *src)>> move_ctor,
            std::optional<std::function<void(void *)>> dtor);

    template <typename T>
    typename std::enable_if<std::is_class_v<T>, BoundTypeDef>::type create_type_def(const std::string &name) {
        // ideally we would emit a warning here if the class doesn't derive from ScriptBindable

        std::optional<std::function<void(void *, const void *)>> copy_ctor;
        std::optional<std::function<void(void *, void *)>> move_ctor;
        std::optional<std::function<void(void *)>> dtor;

        if constexpr (std::is_copy_constructible_v<T>) {
            copy_ctor = [](void *dst, const void *src) {
                new(dst) T(*reinterpret_cast<const T *>(src));
            };
        }

        if constexpr (std::is_move_constructible_v<T>) {
            move_ctor = [](void *dst, void *src) {
                T &rhs = *reinterpret_cast<T *>(src);
                new(dst) T(std::move(rhs));
            };
        }

        if constexpr (std::is_destructible_v<T>) {
            dtor = [](void *obj) { reinterpret_cast<T *>(obj)->~T(); };
        }

        return create_type_def(name, sizeof(T), typeid(T),
                copy_ctor,
                move_ctor,
                dtor);
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

    void bind_member_instance_function(std::type_index type_index, const BoundFunctionDef &fn_def);

    template <typename FuncType>
    typename std::enable_if<std::is_member_function_pointer_v<FuncType>, void>::type
    bind_member_instance_function(const std::string &fn_name, FuncType fn) {
        using ClassType = typename function_traits<FuncType>::class_type;
        static_assert(!std::is_void_v<ClassType>, "Loose function cannot be passed to bind_member_instance_function");

        auto fn_def = _create_function_def(fn_name, fn, FunctionType::MemberInstance);
        bind_member_instance_function(typeid(ClassType), fn_def);
    }

    void add_member_static_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def);

    template <typename FuncType>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>, void>::type
    add_member_static_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def(fn_name, fn, FunctionType::MemberStatic);
        add_member_static_function(type_def, fn_def);
    }

    void bind_member_static_function(std::type_index type_index, const BoundFunctionDef &fn_def);

    template <typename FuncType>
    typename std::enable_if<std::is_member_function_pointer_v<FuncType>, void>::type
    bind_member_static_function(const std::string &fn_name, FuncType fn) {
        using ClassType = typename function_traits<FuncType>::class_type;
        static_assert(!std::is_void_v<ClassType>, "Loose function cannot be passed to bind_member_static_function");

        auto fn_def = _create_function_def(fn_name, fn, FunctionType::MemberInstance);
        bind_member_static_function(typeid(ClassType), fn_def);
    }

    void add_member_field(BoundTypeDef &type_def, const BoundFieldDef &field_def);

    template <typename FieldRef>
    typename std::enable_if<std::is_member_object_pointer_v<FieldRef>, void>::type
    add_member_field(BoundTypeDef &type_def, const std::string &field_name, FieldRef field) {
        auto field_def = _create_field_def(field_name, field);
        add_member_field(type_def, field_def);
    }

    void bind_member_field(std::type_index type_index, const BoundFieldDef &field_def);

    template <typename FieldType, typename ClassType>
    void bind_member_field(const std::string &field_name, FieldType(ClassType::* field)) {
        auto field_def = _create_field_def(field_name, field);
        bind_member_field(typeid(ClassType), field_def);
    }

    BoundEnumDef create_enum_def(const std::string &name, size_t width, std::type_index type_index);

    template <typename E>
    typename std::enable_if<std::is_enum_v<E>, BoundEnumDef>::type
    create_enum_def(const std::string &name) {
        return create_enum_def(name, sizeof(std::underlying_type_t<E>),
                typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<E>>>));
    }

    void add_enum_value(BoundEnumDef &def, const std::string &name, uint64_t value);

    template <typename T>
    typename std::enable_if<std::is_enum_v<T>>::type
    add_enum_value(BoundEnumDef &def, const std::string &name, T value) {
        if constexpr (std::is_signed_v<std::underlying_type_t<T>>) {
            add_enum_value(def, name, uint64_t(value));
        } else {
            add_enum_value(def, name, uint64_t(value));
        }
    }

    void bind_enum_value(std::type_index enum_type, const std::string &name, uint64_t value);

    template <typename T>
    typename std::enable_if<std::is_enum_v<T>>::type
    bind_enum_value(const std::string &name, T value) {
        std::type_index enum_type = typeid(T);
        if constexpr (std::is_signed_v<std::underlying_type_t<T>>) {
            bind_enum_value(enum_type, name, uint64_t(value));
        } else {
            bind_enum_value(enum_type, name, uint64_t(value));
        }
    }
}

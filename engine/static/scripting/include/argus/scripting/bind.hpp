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
#include "argus/lowlevel/misc.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/exception.hpp"
#include "argus/scripting/object_type.hpp"
#include "argus/scripting/types.hpp"
#include "argus/scripting/wrapper.hpp"

#include <functional>
#include <string>

namespace argus {
    BoundTypeDef create_type_def(const std::string &name, size_t size, std::type_index type_index, bool is_refable,
            CopyCtorProxy copy_ctor,
            MoveCtorProxy move_ctor,
            DtorProxy dtor);

    template <typename T>
    typename std::enable_if<std::is_class_v<T>, BoundTypeDef>::type create_type_def(const std::string &name) {
        // ideally we would emit a warning here if the class doesn't derive from AutoCleanupable

        constexpr bool is_refable = std::is_base_of_v<AutoCleanupable, T>;

        CopyCtorProxy copy_ctor = nullptr;
        MoveCtorProxy move_ctor = nullptr;
        DtorProxy dtor = nullptr;

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

        return create_type_def(name, sizeof(T), typeid(T), is_refable,
                copy_ctor,
                move_ctor,
                dtor);
    }

    void bind_type(const BoundTypeDef &def);

    template <typename T>
    typename std::enable_if<std::is_class_v<T>, void>::type bind_type(const std::string &name) {
        // ideally we would emit a warning here if the class doesn't derive from AutoCleanupable
        auto def = create_type_def<T>(name);
        bind_type(def);
    }

    void bind_enum(const BoundEnumDef &def);

    BoundEnumDef create_enum_def(const std::string &name, size_t width, std::type_index type_index);

    template <typename E>
    typename std::enable_if<std::is_enum_v<E>, BoundEnumDef>::type
    create_enum_def(const std::string &name) {
        return create_enum_def(name, sizeof(std::underlying_type_t<E>),
                typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<E>>>));
    }

    template <typename T>
    typename std::enable_if<std::is_enum_v<T>, void>::type bind_enum(const std::string &name) {
        auto def = create_enum_def<T>(name);
        bind_enum(def);
    }

    void add_enum_value(BoundEnumDef &def, const std::string &name, uint64_t value);

    template <typename T>
    typename std::enable_if<std::is_enum_v<T>>::type
    add_enum_value(BoundEnumDef &def, const std::string &name, T value) {
        add_enum_value(def, name, uint64_t(value));
    }

    void bind_enum_value(std::type_index enum_type, const std::string &name, uint64_t value);

    template <typename T>
    typename std::enable_if<std::is_enum_v<T>>::type
    bind_enum_value(const std::string &name, T value) {
        std::type_index enum_type = typeid(T);
        bind_enum_value(enum_type, name, uint64_t(value));
    }

    template <typename FuncType>
    ProxiedFunction create_function_wrapper(FuncType fn) {
        using ReturnType = typename function_traits<FuncType>::return_type;
        if constexpr (!std::is_void_v<ReturnType>) {
            return [fn] (const std::vector<ObjectWrapper> &params) {
                ReturnType ret = invoke_function(fn, params);

                auto ret_obj_type = create_return_object_type<ReturnType>();
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
                } else if ((ret_obj_type.type == IntegralType::Vector || ret_obj_type.type == IntegralType::VectorRef)
                        && (ret_obj_type.element_type.value()->type == IntegralType::Struct
                        || ret_obj_type.element_type.value()->type == IntegralType::Pointer)) {
                    ret_obj_type.element_type.value()->type_name
                            = get_bound_type(ret_obj_type.element_type.value()->type_index.value()).name;
                } else if ((ret_obj_type.type == IntegralType::Vector || ret_obj_type.type == IntegralType::VectorRef)
                        && ret_obj_type.element_type.value()->type == IntegralType::Enum) {
                    ret_obj_type.element_type.value()->type_name
                            = get_bound_enum(ret_obj_type.element_type.value()->type_index.value()).name;
                }

                ObjectWrapper wrapper(ret_obj_type, ret_obj_size);
                if constexpr (std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<ReturnType>>>, std::string>) {
                    return create_object_wrapper(ret_obj_type, ret.c_str(), ret.size());
                } else if constexpr (std::is_pointer_v<std::remove_cv_t<std::remove_reference_t<ReturnType>>>
                        && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<
                        std::remove_reference_t<ReturnType>>>, char>) {
                    return create_object_wrapper(ret_obj_type, ret, strlen(ret));
                } else if constexpr (is_std_vector_v<ReturnType>) {
                    return create_vector_object_wrapper_from_stack(ret_obj_type, ret);
                } else if constexpr (is_std_vector_v<std::remove_cv_t<
                        std::remove_reference_t<std::remove_pointer_t<ReturnType>>>>) {
                    return create_vector_object_wrapper_from_heap(ret_obj_type, ret);
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
                auto type = create_return_object_type<void>();
                return ObjectWrapper(type, 0);
            };
        }
    }

    template <FunctionType FnType, typename FnSig>
    static BoundFunctionDef _create_function_def(const std::string &name, FnSig fn) {
        using ArgsTuple = typename function_traits<FnSig>::argument_types;
        using ReturnType = typename function_traits<FnSig>::return_type;
        bool is_const;
        if constexpr (FnType == FunctionType::Extension) {
            static_assert(std::tuple_size_v<ArgsTuple> > 0);
            is_const = std::is_const_v<std::remove_reference_t<std::remove_pointer_t<
                    std::tuple_element_t<0, ArgsTuple>>>>;
        } else {
            is_const = function_traits<FnSig>::is_const::value;
        }

        try {
            BoundFunctionDef def {
                    name,
                    FnType,
                    is_const,
                    // Callback return values flow from the script VM to C++, so
                    // we don't need to worry about invalidating any handles.
                    tuple_to_object_types<ArgsTuple, DataFlowDirection::FromScript>(),
                    create_return_object_type<ReturnType>(),
                    create_function_wrapper(fn)
            };

            return def;
        } catch (const std::exception &ex) {
            throw BindingException(name, ex.what());
        }
    }

    template <typename FuncType>
    BoundFunctionDef create_global_function_def(const std::string &name, FuncType fn) {
        return _create_function_def<FunctionType::Global>(name, fn);
    }

    void bind_global_function(const BoundFunctionDef &def);

    template <typename FuncType>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>, void>::type
    bind_global_function(const std::string &name, FuncType fn) {
        auto def = create_global_function_def<FuncType>(name, fn);
        bind_global_function(def);
    }

    void add_member_instance_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def);

    template <typename FuncType>
    typename std::enable_if<std::is_member_function_pointer_v<FuncType>, void>::type
    add_member_instance_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def<FunctionType::MemberInstance>(fn_name, fn);
        add_member_instance_function(type_def, fn_def);
    }

    void bind_member_instance_function(std::type_index type_index, const BoundFunctionDef &fn_def);

    template <typename FuncType>
    typename std::enable_if<std::is_member_function_pointer_v<FuncType>, void>::type
    bind_member_instance_function(const std::string &fn_name, FuncType fn) {
        using ClassType = typename function_traits<FuncType>::class_type;
        static_assert(!std::is_void_v<ClassType>, "Loose function cannot be passed to bind_member_instance_function");

        auto fn_def = _create_function_def<FunctionType::MemberInstance>(fn_name, fn);
        bind_member_instance_function(typeid(ClassType), fn_def);
    }

    void add_member_static_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def);

    template <typename FuncType>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>, void>::type
    add_member_static_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def<FunctionType::MemberStatic>(fn_name, fn);
        add_member_static_function(type_def, fn_def);
    }

    void bind_member_static_function(std::type_index type_index, const BoundFunctionDef &fn_def);

    template <typename ClassType, typename FuncType>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>, void>::type
    bind_member_static_function(const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def<FunctionType::MemberStatic>(fn_name, fn);
        bind_member_static_function(typeid(ClassType), fn_def);
    }

    void add_extension_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def);

    template <typename ClassType, typename FuncType,
            typename FirstArg = typename std::tuple_element_t<0, typename function_traits<FuncType>::argument_types>>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>
            && std::is_same_v<ClassType, std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<FirstArg>>>>,
            void>::type
    add_extension_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def<FunctionType::Extension>(fn_name, fn);
        add_extension_function(type_def, fn_def);
    }

    void bind_extension_function(std::type_index type_index, const BoundFunctionDef &fn_def);

    template <typename ClassType, typename FuncType,
            typename FirstArg = typename std::tuple_element_t<0, typename function_traits<FuncType>::argument_types>>
    typename std::enable_if<!std::is_member_function_pointer_v<FuncType>
            && (std::is_reference_v<FirstArg> || std::is_pointer_v<FirstArg>)
            && std::is_same_v<ClassType, std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<FirstArg>>>>,
            void>::type
    bind_extension_function(const std::string &fn_name, FuncType fn) {
        auto fn_def = _create_function_def<FunctionType::Extension>(fn_name, fn);
        bind_extension_function(typeid(ClassType), fn_def);
    }

    template <typename FieldType, typename ClassType>
    static BoundFieldDef _create_field_def(const std::string &name, FieldType(ClassType::* field)) {
        using B = typename std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<FieldType>>>;

        static_assert(std::is_base_of_v<AutoCleanupable, B>
                        || (std::is_copy_constructible_v<B> && std::is_move_constructible_v<B> && std::is_destructible_v<B>),
                "Type of bound field must either derive from AutoCleanupable "
                "or be destructible and copy+move-constructible");

        // treat C-string fields as const because there's no good way to manage their memory
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<FieldType>>
                || std::is_same_v<std::remove_cv_t<FieldType>, char *>;

        using B = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<FieldType>>>;

        try {
            // field values are passed from the engine to the script VM
            // if the field isn't refable it will always be copied by value, so
            // we need to pretend it's a non-pointer when creating the object
            // type and doing the relevant checks
            ObjectType type;
            if constexpr (!std::is_class_v<B> || std::is_base_of_v<AutoCleanupable, B>) {
                type = create_object_type<FieldType, DataFlowDirection::ToScript>();
            } else {
                type = create_object_type<B, DataFlowDirection::ToScript>();
            }
            type.is_const = is_const;

            BoundFieldDef def{};
            def.m_name = name;
            def.m_type = type;

            def.m_access_proxy = [field](ObjectWrapper &inst, const ObjectType &field_type) {
                ClassType *instance = reinterpret_cast<ClassType *>(inst.is_on_heap
                        ? inst.heap_ptr
                        : inst.stored_ptr);

                auto real_type = field_type;
                if constexpr ((std::is_class_v<FieldType>
                        || (std::is_class_v<B> && !std::is_base_of_v<AutoCleanupable, B>))
                        && !std::is_same_v<B, std::string>
                        && !is_std_vector_v<B>
                        && !is_std_function_v<B>) {
                    assert(real_type.type == IntegralType::Struct);
                    if constexpr (std::is_base_of_v<AutoCleanupable, B>) {
                        // return a pointer to the field so the script can modify its fields
                        real_type.type = IntegralType::Pointer;
                        real_type.size = sizeof(void *);
                        return create_auto_object_wrapper(real_type, &(instance->*field));
                    } else {
                        // return a copy of the field since we can't manage a reference to it
                        real_type.is_const = true;
                        return create_auto_object_wrapper(real_type, (instance->*field));
                    }
                } else {
                    // return the field by value
                    return create_auto_object_wrapper(real_type, instance->*field);
                }
            };

            if constexpr (!is_const) {
                def.m_assign_proxy = [field](ObjectWrapper &inst, ObjectWrapper &val) {
                    ClassType *instance = reinterpret_cast<ClassType *>(inst.is_on_heap
                            ? inst.heap_ptr
                            : inst.stored_ptr);

                    instance->*field = unwrap_param<FieldType>(val, nullptr);
                };
            }

            return def;
        } catch (const std::exception &ex) {
            throw BindingException(name, ex.what());
        }
    }

    void add_member_field(BoundTypeDef &type_def, const BoundFieldDef &field_def);

    template <typename FieldType, typename ClassType>
    void add_member_field(BoundTypeDef &type_def, const std::string &field_name, FieldType(ClassType::* field)) {
        static_assert(!std::is_function_v<FieldType> && !is_std_function_v<FieldType>,
                "Callback-typed fields are not supported");

        affirm_precond(std::type_index(typeid(ClassType)) == type_def.type_index,
                "Class of field reference does not match provided type definition");

        auto field_def = _create_field_def(field_name, field);
        add_member_field(type_def, field_def);
    }

    void bind_member_field(std::type_index type_index, const BoundFieldDef &field_def);

    template <typename FieldType, typename ClassType>
    void bind_member_field(const std::string &field_name, FieldType(ClassType::* field)) {
        static_assert(!std::is_function_v<FieldType> && !is_std_function_v<FieldType>,
                "Callback-typed fields are not supported");

        auto field_def = _create_field_def(field_name, field);
        bind_member_field(typeid(ClassType), field_def);
    }

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
}

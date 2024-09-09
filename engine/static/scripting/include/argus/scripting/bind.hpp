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
#include "argus/lowlevel/result.hpp"

#include "argus/core/engine.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/error.hpp"
#include "argus/scripting/object_type.hpp"
#include "argus/scripting/types.hpp"
#include "argus/scripting/wrapper.hpp"

#include <functional>
#include <string>

namespace argus {
    class Transform2D;

    [[nodiscard]] Result<BoundTypeDef, BindingError> create_type_def(const std::string &name, size_t size,
            const std::string &type_id, bool is_refable,
            CopyCtorProxy copy_ctor,
            MoveCtorProxy move_ctor,
            DtorProxy dtor);

    template<typename T>
    [[nodiscard]] std::enable_if_t<std::is_class_v<T>, Result<BoundTypeDef, BindingError>>
    create_type_def(const std::string &name) {
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
            dtor = [](void *ptr) {
                static_cast<T *>(ptr)->~T();
            };
        }

        return create_type_def(name, sizeof(T), typeid(T).name(), is_refable,
                copy_ctor,
                move_ctor,
                dtor);
    }

    [[nodiscard]] Result<void, BindingError> bind_type(const BoundTypeDef &def);

    template<typename T>
    [[nodiscard]] std::enable_if_t<std::is_class_v<T>, Result<void, BindingError>>
    bind_type(const std::string &name) {
        // ideally we would emit a warning here if the class doesn't derive from AutoCleanupable
        return create_type_def<T>(name)
                .template and_then<void>([](const auto &fn_def) {
                    return bind_type(fn_def);
                });
    }

    [[nodiscard]] Result<void, BindingError> bind_enum(const BoundEnumDef &def);

    [[nodiscard]] Result<BoundEnumDef, BindingError> create_enum_def(const std::string &name, size_t width,
            const std::string &type_id);

    template<typename E>
    [[nodiscard]] std::enable_if_t<std::is_enum_v<E>, Result<BoundEnumDef, BindingError>>
    create_enum_def(const std::string &name) {
        return create_enum_def(name, sizeof(std::underlying_type_t<E>),
                typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<E>>>).name());
    }

    template<typename T>
    [[nodiscard]] std::enable_if_t<std::is_enum_v<T>, Result<void, BindingError>> bind_enum(
            const std::string &name) {
        return create_enum_def<T>(name)
                .template and_then<void>([](const auto &def) {
                    return bind_enum(def);
                });
    }

    [[nodiscard]] Result<void, BindingError> add_enum_value(BoundEnumDef &def, const std::string &name, int64_t value);

    template<typename T>
    [[nodiscard]] std::enable_if_t<std::is_enum_v<T>, Result<void, BindingError>>
    add_enum_value(BoundEnumDef &def, const std::string &name, T value) {
        return add_enum_value(def, name, int64_t(value));
    }

    [[nodiscard]] Result<void, BindingError> bind_enum_value(const std::string &enum_type, const std::string &name,
            int64_t value);

    template<typename T>
    [[nodiscard]] std::enable_if_t<std::is_enum_v<T>, Result<void, BindingError>>
    bind_enum_value(const std::string &name, T value) {
        return bind_enum_value(typeid(T).name(), name, int64_t(value));
    }

    template<typename FuncType>
    [[nodiscard]] ProxiedNativeFunction create_function_wrapper(FuncType fn) {
        using ReturnType = typename function_traits<FuncType>::return_type;
        if constexpr (!std::is_void_v<ReturnType>) {
            return [fn](std::vector<ObjectWrapper> &params) {
                auto ret_res = invoke_function(fn, params);
                if (ret_res.is_err()) {
                    return err<ObjectWrapper, ReflectiveArgumentsError>(ret_res.unwrap_err());
                }
                ReturnType ret = ret_res.unwrap();

                auto ret_obj_type = create_return_object_type<ReturnType>();
                size_t ret_obj_size;

                // Older versions (<= 9) of GCC are too stupid to work out that
                // this var doesn't need to be assigned in a branch that never
                // returns to the parent scope, so we explicitly mark it unused
                // here to prevent a compile error.
                UNUSED(ret_obj_size);

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
                            static_assert(always_false<FuncType>, "Unexpected pointer type for function return type");
                        }
                    } else if constexpr (std::is_same_v<std::remove_cv_t<ReturnType>, std::string>) {
                        ret_obj_size = ret.length();
                    } else {
                        static_assert(always_false<FuncType>, "Unexpected type for function return type");
                    }
                } else {
                    ret_obj_size = sizeof(ReturnType);
                }

                // _create_object_type is used to create function definitions
                // too so it doesn't attempt to resolve the type name
                //TODO: figure out if we can move this to the .cpp file
                switch (ret_obj_type.type) {
                    case IntegralType::Pointer:
                    case IntegralType::Struct: {
                        ret_obj_type.type_name = get_bound_type<ReturnType>()
                                .expect("Tried to create function wrapper with unbound return struct type").name;

                        break;
                    }
                    case IntegralType::Enum: {
                        ret_obj_type.type_name = get_bound_enum<ReturnType>()
                                .expect("Tried to create function wrapper with unbound return enum type").name;

                        break;
                    }
                    case IntegralType::Vector:
                    case IntegralType::VectorRef: {
                        if (ret_obj_type.primary_type.value()->type == IntegralType::Struct
                                || ret_obj_type.primary_type.value()->type == IntegralType::Pointer) {
                            ret_obj_type.primary_type.value()->type_name
                                    = get_bound_type(ret_obj_type.primary_type.value()->type_id.value())
                                    .expect("Tried to create function wrapper with vector return type and "
                                            "unbound element struct type").name;
                        } else if (ret_obj_type.primary_type.value()->type == IntegralType::Enum) {
                            ret_obj_type.primary_type.value()->type_name
                                    = get_bound_enum(ret_obj_type.primary_type.value()->type_id.value())
                                    .expect("Tried to create function wrapper with vector return type and "
                                            "unbound element enum type").name;
                        }

                        break;
                    }
                    case IntegralType::Result: {
                        argus_assert(ret_obj_type.primary_type.has_value());
                        argus_assert(ret_obj_type.secondary_type.has_value());

                        if (ret_obj_type.primary_type.value()->type == IntegralType::Struct
                                || ret_obj_type.primary_type.value()->type == IntegralType::Pointer) {
                            ret_obj_type.primary_type.value()->type_name
                                    = get_bound_type(ret_obj_type.primary_type.value()->type_id.value())
                                            .expect("Tried to create function wrapper with result return type and "
                                                    "unbound value struct type").name;
                        } else if (ret_obj_type.primary_type.value()->type == IntegralType::Enum) {
                            ret_obj_type.primary_type.value()->type_name
                                    = get_bound_type(ret_obj_type.primary_type.value()->type_id.value())
                                            .expect("Tried to create function wrapper with result return type and "
                                                    "unbound value enum type").name;
                        }

                        if (ret_obj_type.secondary_type.value()->type == IntegralType::Struct
                                || ret_obj_type.secondary_type.value()->type == IntegralType::Pointer) {
                            ret_obj_type.secondary_type.value()->type_name
                                    = get_bound_type(ret_obj_type.secondary_type.value()->type_id.value())
                                            .expect("Tried to create function wrapper with result return type and "
                                                    "unbound error struct type").name;
                        } else if (ret_obj_type.secondary_type.value()->type == IntegralType::Enum) {
                            ret_obj_type.secondary_type.value()->type_name
                                    = get_bound_type(ret_obj_type.secondary_type.value()->type_id.value())
                                            .expect("Tried to create function wrapper with result return type and "
                                                    "unbound error enum type").name;
                        }

                        break;
                    }
                    default:
                        break; // nothing to do
                }

                if constexpr (std::is_same_v<std::remove_cv_t<std::remove_pointer_t<
                        std::remove_reference_t<ReturnType>>>, std::string>) {
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
                } else if constexpr (is_result_v<std::remove_cv_t<ReturnType>>) {
                    return create_result_object_wrapper(ret_obj_type, ret);
                } else if constexpr (std::is_reference_v<ReturnType>) {
                    ObjectWrapper wrapper(ret_obj_type, ret_obj_size);

                    void *ptr = const_cast<void *>(reinterpret_cast<const void *>(&ret));
                    wrapper.copy_value_from(&ptr, sizeof(void *));

                    return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
                } else if constexpr (std::is_pointer_v<ReturnType>) {
                    ObjectWrapper wrapper(ret_obj_type, ret_obj_size);

                    void *ptr = const_cast<void *>(reinterpret_cast<const void *>(ret));
                    wrapper.copy_value_from(&ptr, sizeof(void *));

                    return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
                } else {
                    return create_object_wrapper(ret_obj_type, &ret, sizeof(ReturnType));
                }
            };
        } else {
            return [fn](std::vector<ObjectWrapper> &params) {
                invoke_function(fn, params);
                auto type = create_return_object_type<void>();
                return ok<ObjectWrapper, ReflectiveArgumentsError>(ObjectWrapper(type, 0));
            };
        }
    }

    template<FunctionType FnType, typename FnSig>
    [[nodiscard]] static Result<BoundFunctionDef, BindingError> _create_function_def(const std::string &name,
            FnSig fn) {
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

        return ok<BoundFunctionDef, BindingError>(def);
    }

    template<typename FuncType>
    [[nodiscard]] Result<BoundFunctionDef, BindingError> create_global_function_def(const std::string &name,
            FuncType fn) {
        return _create_function_def<FunctionType::Global>(name, fn);
    }

    [[nodiscard]] Result<void, BindingError> bind_global_function(const BoundFunctionDef &def);

    template<typename FuncType>
    [[nodiscard]] std::enable_if_t<!std::is_member_function_pointer_v<FuncType>, Result<void, BindingError>>
    bind_global_function(const std::string &name, FuncType fn) {
        return create_global_function_def<FuncType>(name, fn)
                .template and_then<void>([](const auto &def) {
                    return bind_global_function(def);
                });
    }

    [[nodiscard]] Result<void, BindingError> add_member_instance_function(BoundTypeDef &type_def,
            const BoundFunctionDef &fn_def);

    template<typename FuncType>
    [[nodiscard]] std::enable_if_t<std::is_member_function_pointer_v<FuncType>, Result<void, BindingError>>
    add_member_instance_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        return _create_function_def<FunctionType::MemberInstance>(fn_name, fn)
                .template and_then<void>([&type_def](const auto &fn_def) {
                    return add_member_instance_function(type_def, fn_def.unwrap());
                });
    }

    [[nodiscard]] Result<void, BindingError> bind_member_instance_function(const std::string &type_id,
            const BoundFunctionDef &fn_def);

    template<typename FuncType>
    [[nodiscard]] std::enable_if_t<std::is_member_function_pointer_v<FuncType>, Result<void, BindingError>>
    bind_member_instance_function(const std::string &fn_name, FuncType fn) {
        using ClassType = typename function_traits<FuncType>::class_type;
        static_assert(!std::is_void_v<ClassType>, "Loose function cannot be passed to bind_member_instance_function");

        return _create_function_def<FunctionType::MemberInstance>(fn_name, fn)
                .template and_then<void>([](const auto &fn_def) {
                    return bind_member_instance_function(typeid(ClassType).name(), fn_def);
                });
    }

    [[nodiscard]] Result<void, BindingError> add_member_static_function(BoundTypeDef &type_def,
            const BoundFunctionDef &fn_def);

    template<typename FuncType>
    [[nodiscard]] std::enable_if_t<!std::is_member_function_pointer_v<FuncType>, Result<void, BindingError>>
    add_member_static_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        return _create_function_def<FunctionType::MemberStatic>(fn_name, fn)
                .template and_then<void>([&type_def](const auto &fn_def) {
                    return add_member_static_function(type_def, fn_def.unwrap());
                });
    }

    [[nodiscard]] Result<void, BindingError> bind_member_static_function(const std::string &type_id,
            const BoundFunctionDef &fn_def);

    template<typename ClassType, typename FuncType>
    [[nodiscard]] std::enable_if_t<!std::is_member_function_pointer_v<FuncType>, Result<void, BindingError>>
    bind_member_static_function(const std::string &fn_name, FuncType fn) {
        return _create_function_def<FunctionType::MemberStatic>(fn_name, fn)
                .template and_then<void>([](const auto &fn_def) {
                    return bind_member_static_function(typeid(ClassType).name(), fn_def);
                });
    }

    [[nodiscard]] Result<void, BindingError> add_extension_function(BoundTypeDef &type_def,
            const BoundFunctionDef &fn_def);

    template<typename ClassType, typename FuncType,
            typename FirstArg = typename std::tuple_element_t<0, typename function_traits<FuncType>::argument_types>>
    [[nodiscard]] std::enable_if_t<!std::is_member_function_pointer_v<FuncType>
            && std::is_same_v<ClassType, std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<FirstArg>>>>,
            Result<void, BindingError>>
    add_extension_function(BoundTypeDef &type_def, const std::string &fn_name, FuncType fn) {
        return _create_function_def<FunctionType::Extension>(fn_name, fn)
                .template and_then<void>([&type_def](const auto &fn_def) {
                    return add_extension_function(type_def, fn_def);
                });
    }

    [[nodiscard]] Result<void, BindingError> bind_extension_function(const std::string &type_id,
            const BoundFunctionDef &fn_def);

    template<typename ClassType, typename FuncType,
            typename FirstArg = typename std::tuple_element_t<0, typename function_traits<FuncType>::argument_types>>
    [[nodiscard]] std::enable_if_t<!std::is_member_function_pointer_v<FuncType>
            && (std::is_reference_v<FirstArg> || std::is_pointer_v<FirstArg>)
            && std::is_same_v<ClassType, std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<FirstArg>>>>,
            Result<void, BindingError>>
    bind_extension_function(const std::string &fn_name, FuncType fn) {
        return _create_function_def<FunctionType::Extension>(fn_name, fn)
                .template and_then<void>([](const auto &fn_def) {
                    return bind_extension_function(typeid(ClassType).name(), fn_def);
                });
    }

    template<typename FieldType, typename ClassType>
    [[nodiscard]] static Result<BoundFieldDef, BindingError> _create_field_def(const std::string &name,
            FieldType(ClassType::* field)) {
        using B = typename std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<FieldType>>>;

        static_assert(std::is_base_of_v<AutoCleanupable, B>
                        || (std::is_copy_constructible_v<B>
                                && std::is_move_constructible_v<B>
                                && std::is_destructible_v<B>),
                "Type of bound field must either derive from AutoCleanupable "
                "or be destructible and copy+move-constructible");

        // treat C-string fields as const because there's no good way to manage their memory
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<FieldType>>
                || std::is_same_v<std::remove_cv_t<FieldType>, char *>;

        using B = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<FieldType>>>;

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

        BoundFieldDef def {};
        def.m_name = name;
        def.m_type = type;

        def.m_access_proxy = [name, field](ObjectWrapper &inst, const ObjectType &field_type) {
            ClassType &instance = inst.get_value<ClassType &>();

            auto real_type = field_type;
            if constexpr ((std::is_class_v<FieldType>
                    || (std::is_class_v<B> && !std::is_base_of_v<AutoCleanupable, B>))
                    && !std::is_same_v<B, std::string>
                    && !is_std_vector_v<B>
                    && !is_std_function_v<B>) {
                argus_assert(real_type.type == IntegralType::Struct);
                if constexpr (std::is_base_of_v<AutoCleanupable, B>) {
                    // return a pointer to the field so the script can modify its fields
                    real_type.type = IntegralType::Pointer;
                    real_type.size = sizeof(void *);
                    return std::move(create_auto_object_wrapper(real_type, &(instance.*field))
                            .expect("Failed to create object wrapper while accessing native field "
                                    + name + " from script"));
                } else {
                    // return a copy of the field since we can't manage a reference to it
                    real_type.is_const = true;
                    return std::move(create_auto_object_wrapper(real_type, (instance.*field))
                            .expect("Failed to create object wrapper while accessing native field "
                                    + name + " from script"));
                }
            } else {
                // return the field by value
                return std::move(create_auto_object_wrapper(real_type, instance.*field)
                        .expect("Failed to create object wrapper while accessing native field "
                                + name + " from script"));
            }
        };

        if constexpr (!is_const) {
            def.m_assign_proxy = [field](ObjectWrapper &inst, ObjectWrapper &val) {
                ClassType &instance = inst.get_value<ClassType &>();

                instance.*field = unwrap_param<FieldType>(val, nullptr);
            };
        }

        return ok<BoundFieldDef, BindingError>(def);
    }

    [[nodiscard]] Result<void, BindingError> add_member_field(BoundTypeDef &type_def, const BoundFieldDef &field_def);

    template<typename FieldType, typename ClassType>
    [[nodiscard]] Result<void, BindingError> add_member_field(BoundTypeDef &type_def, const std::string &field_name,
            FieldType(ClassType::* field)) {
        static_assert(!std::is_function_v<FieldType> && !is_std_function_v<FieldType>,
                "Callback-typed fields are not supported");

        affirm_precond(typeid(ClassType).name() == type_def.type_id,
                "Class of field reference does not match provided type definition");

        auto field_def = _create_field_def(field_name, field)
                .template and_then<void>([&type_def](const auto field_def) {
                    return add_member_field(type_def, field_def);
                });
    }

    [[nodiscard]] Result<void, BindingError> bind_member_field(const std::string &type_id,
            const BoundFieldDef &field_def);

    template<typename FieldType, typename ClassType>
    [[nodiscard]] Result<void, BindingError> bind_member_field(const std::string &field_name,
            FieldType(ClassType::* field)) {
        static_assert(!std::is_function_v<FieldType> && !is_std_function_v<FieldType>,
                "Callback-typed fields are not supported");

        return _create_field_def(field_name, field)
                .template and_then<void>([](const auto &field_def) {
                    return bind_member_field(typeid(ClassType).name(), field_def);
                });
    }

    [[nodiscard]] Result<const BoundTypeDef &, BindingError> get_bound_type_by_name(const std::string &type_name);

    [[nodiscard]] Result<const BoundTypeDef &, BindingError> get_bound_type(const std::string &type_id);

    template<typename T>
    [[nodiscard]] Result<const BoundTypeDef &, BindingError> get_bound_type(void) {
        return get_bound_type(typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>).name());
    }

    [[nodiscard]] Result<const BoundEnumDef &, BindingError> get_bound_enum_by_name(const std::string &enum_name);

    [[nodiscard]] Result<const BoundEnumDef &, BindingError> get_bound_enum(const std::string &enum_type_id);

    template<typename T>
    [[nodiscard]] Result<const BoundEnumDef &, BindingError> get_bound_enum(void) {
        return get_bound_enum(typeid(std::remove_const_t<T>).name());
    }
}

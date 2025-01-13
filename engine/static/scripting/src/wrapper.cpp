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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/result.hpp"

#include "argus/core/engine.hpp"

#include "argus/scripting/bind.hpp"
#include "argus/scripting/error.hpp"
#include "argus/scripting/wrapper.hpp"

#include <string>

#include <cstring>

namespace argus {
    Result<ObjectWrapper, ReflectiveArgumentsError> create_object_wrapper(const ObjectType &type, const void *ptr,
            size_t size) {
        ObjectWrapper wrapper(type, size);
        wrapper.copy_value_from(ptr, size);

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_object_wrapper(const ObjectType &type, const void *ptr) {
        affirm_precond(type.type != IntegralType::String, "Cannot create object wrapper for string-typed value - "
                                                          "string-specific overload must be used");

        return create_object_wrapper(type, ptr, type.size);
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_int_object_wrapper(const ObjectType &type, int64_t val) {
        auto is_signed = type.type == IntegralType::Integer || type.type == IntegralType::Enum;
        ObjectWrapper wrapper(type, type.size);
        argus_assert(wrapper.buffer_size >= type.size);

        if (type.type == IntegralType::Enum) {
            argus_assert(type.type_name.has_value());
            auto enum_def = get_bound_enum(type.type_id.value())
                    .expect("Tried to create ObjectWrapper with unbound enum type");
            auto enum_val_it = enum_def.all_ordinals.find(*reinterpret_cast<int64_t *>(&val));
            if (enum_val_it == enum_def.all_ordinals.cend()) {
                return err<ObjectWrapper, ReflectiveArgumentsError>("Unknown ordinal " + std::to_string(val)
                        + " for enum type " + enum_def.name);
            }
        }

        switch (type.size) {
            case 1: {
                if (is_signed) {
                    wrapper.store_value(int8_t(val));
                } else {
                    wrapper.store_value(uint8_t(val));
                }
                break;
            }
            case 2: {
                if (is_signed) {
                    wrapper.store_value(int16_t(val));
                } else {
                    wrapper.store_value(uint16_t(val));
                }
                break;
            }
            case 4: {
                if (is_signed) {
                    wrapper.store_value(int32_t(val));
                } else {
                    wrapper.store_value(uint32_t(val));
                }
                break;
            }
            case 8: {
                if (is_signed) {
                    wrapper.store_value(int64_t(val));
                } else {
                    wrapper.store_value(uint64_t(val));
                }
                break;
            }
            default:
                argus_assert(false); // should have been caught during binding
        }

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_float_object_wrapper(const ObjectType &type, double val) {
        ObjectWrapper wrapper(type, type.size);

        switch (type.size) {
            case 4: {
                wrapper.store_value(float(val));
                break;
            }
            case 8: {
                wrapper.store_value(double(val));
                break;
            }
            default:
                argus_assert(false); // should have been caught during binding
        }

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_bool_object_wrapper(const ObjectType &type, bool val) {
        argus_assert(type.size >= sizeof(bool));

        ObjectWrapper wrapper(type, type.size);

        wrapper.store_value(val);

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_enum_object_wrapper(const ObjectType &type, int64_t val) {
        return create_int_object_wrapper(type, val);
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_string_object_wrapper(const ObjectType &type,
            const std::string &str) {
        affirm_precond(type.type == IntegralType::String, "Cannot create object wrapper (string-specific overload "
                "called for non-string-typed value");

        return create_object_wrapper(type, str.c_str(), str.length() + 1);
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_callback_object_wrapper(
            const ObjectType &type,
            const ProxiedScriptCallback &fn
    ) {
        affirm_precond(type.type == IntegralType::Callback,
                "Cannot create object wrapper "
                "(callback-specific overload called for non-callback-typed value)");

        return create_object_wrapper(type, &fn, sizeof(fn));
    }

    static void _validate_vec_obj_type(ObjectType vec_type) {
        ObjectType &el_type = *vec_type.primary_type.value();

        if (el_type.type == IntegralType::Void) {
            crash("Vectors of void are not supported");
        } else if (el_type.type == IntegralType::Callback) {
            crash("Vectors of callbacks are not supported");
        } else if (el_type.type == IntegralType::Type) {
            crash("Vectors of types are not supported");
        } else if (el_type.type == IntegralType::Vector) {
            crash("Vectors of vectors are not supported");
        } else if (el_type.type == IntegralType::Boolean) {
            // C++ is stupid and specializes std::vector<bool> as a bitfield
            // which messes up our assumptions about how we can tinker with the
            // vector. Much easier to just not support it.
            crash("Vectors of booleans are not supported");
        }
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_vector_object_wrapper(const ObjectType &vec_type,
            const void *data, size_t count) {
        affirm_precond(vec_type.type == IntegralType::Vector || vec_type.type == IntegralType::VectorRef,
                "Cannot create object wrapper (vector-specific overload called for non-vector-typed value)");

        _validate_vec_obj_type(vec_type);
        affirm_precond(count < SIZE_MAX, "Too many vector elements");

        ObjectType &el_type = *vec_type.primary_type.value();

        bool is_trivially_copyable = el_type.type != IntegralType::String
                && !(el_type.type == IntegralType::Struct
                        && get_bound_type(el_type.type_id.value())
                                .expect("Tried to create ObjectWrapper with unbound type").copy_ctor != nullptr);

        size_t el_size = el_type.size;
        if (el_type.type == IntegralType::String) {
            el_size = sizeof(std::string);
        }

        size_t blob_size = sizeof(ArrayBlob) + el_size * count;

        ObjectWrapper wrapper(vec_type, blob_size);
        ArrayBlob &blob = wrapper.emplace<ArrayBlob>(el_size, count,
                el_type.type == IntegralType::String
                        ? +[](void *ptr) { reinterpret_cast<std::string *>(ptr)->~basic_string(); }
                        : nullptr); 

        if (is_trivially_copyable) {
            // can just copy the whole thing in one go and avoid looping
            memcpy(blob[0], data, el_size * count);
        } else {
            argus_assert(count < SIZE_MAX);

            if (el_type.type == IntegralType::String) {
                // strings need to be handled specially because they're the only
                // non-struct type allowed in a vector that isn't trivially
                // copyable
                auto str_arr = reinterpret_cast<const std::string *>(data);
                for (size_t i = 0; i < count; i++) {
                    const std::string &src_str = str_arr[i];

                    /*auto src_ptr = reinterpret_cast<std::string *>(
                            reinterpret_cast<uintptr_t>(data) + uintptr_t(el_size * i));*/
                    void *dst_ptr = blob[i];
                    //TODO: free memory when wrapper is destroyed
                    new(dst_ptr) std::string(src_str);
                }
            } else {
                argus_assert(el_type.type == IntegralType::Struct);

                auto bound_type_res = get_bound_type(el_type.type_id.value());
                const BoundTypeDef &bound_type = bound_type_res
                        .expect("Tried to create ObjectWrapper with unbound struct type");
                argus_assert(bound_type.copy_ctor != nullptr);

                for (size_t i = 0; i < count; i++) {
                    void *src_ptr = reinterpret_cast<void *>(
                            reinterpret_cast<uintptr_t>(data) + i * uintptr_t(el_size));
                    void *dst_ptr = blob[i];

                    bound_type.copy_ctor(dst_ptr, src_ptr);
                }
            }
        }

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_vector_object_wrapper(const ObjectType &vec_type,
            const VectorWrapper &vec) {
        affirm_precond(vec_type.type == IntegralType::Vector,
                "Cannot create object wrapper (vector-specific overload called for non-vector-typed value)");
        _validate_vec_obj_type(vec_type);

        return create_vector_object_wrapper(vec_type, vec.get_data(), vec.get_size());
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_vector_ref_object_wrapper(const ObjectType &vec_type,
            VectorWrapper vec) {
        affirm_precond(vec_type.type == IntegralType::VectorRef,
                "Cannot create object wrapper (vectorref-specific overload called for non-vectorref-typed value)");
        _validate_vec_obj_type(vec_type);

        ObjectWrapper wrapper(vec_type, sizeof(VectorWrapper));

        wrapper.emplace<VectorWrapper>(std::move(vec));

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_result_object_wrapper(const ObjectType &res_type,
            bool is_ok, const ObjectType &resolved_type, size_t resolved_size, void *resolved_ptr) {
        affirm_precond(res_type.type == IntegralType::Result,
                "Cannot create object wrapper (result-specific overload called for non-result-typed value");

        ObjectWrapper wrapper(res_type, sizeof(ResultWrapper) + resolved_size);
        auto &res_wrapper = wrapper.emplace<ResultWrapper>(is_ok, resolved_size, resolved_type);

        void *real_ptr;
        if (resolved_type.type == IntegralType::Pointer) {
            real_ptr = &resolved_ptr;
        } else {
            real_ptr = resolved_ptr;
        }

        res_wrapper.copy_value_or_error_from(real_ptr);
        return ok<ObjectWrapper, ReflectiveArgumentsError>(wrapper);
    }

    template<bool is_move, typename T = std::conditional_t<is_move, ArrayBlob, const ArrayBlob>>
    static void _copy_or_move_array_blob(const ObjectType &obj_type, void *dst, T &src, size_t max_len) {
        auto el_size = src.element_size();
        auto count = src.size();

        ObjectType &el_type = *obj_type.primary_type.value();

        bool is_trivially_copyable = el_type.type != IntegralType::String
                && !(el_type.type == IntegralType::Struct
                        && get_bound_type(el_type.type_id.value())
                                .expect("Tried to copy/move ArrayBlob with unbound element type").copy_ctor != nullptr);

        size_t blob_size = sizeof(ArrayBlob) + src.element_size() * src.size();

        affirm_precond(max_len >= blob_size, "Can't copy/move ArrayBlob: dest is too small");

        ArrayBlob &new_blob = *new(dst) ArrayBlob(el_size, count, src.element_dtor());

        if (is_trivially_copyable) {
            // can just copy the whole thing in one go and avoid looping
            memcpy(new_blob.data(), src.data(), el_size * count);
        } else {
            argus_assert(count < SIZE_MAX);

            auto bound_type_res = get_bound_type(el_type.type_id.value());
            const BoundTypeDef &bound_type = bound_type_res
                    .expect("Tried to copy/move ArrayBlob with unbound element type");
            argus_assert(bound_type.copy_ctor != nullptr);

            for (size_t i = 0; i < count; i++) {
                std::conditional_t<is_move, void *, const void *> src_ptr = src[i];
                void *dst_ptr = new_blob[i];

                if constexpr (is_move) {
                    move_wrapped_object(el_type, dst_ptr, src_ptr, el_size);
                } else {
                    copy_wrapped_object(el_type, dst_ptr, src_ptr, el_size);
                }
            }
        }
    }

    template<bool is_move, typename T = std::conditional_t<is_move, ResultWrapper, const ResultWrapper>>
    static void _copy_or_move_result_wrapper(void *dst, T &src, size_t max_len) {
        auto &dst_res = *new(dst) ResultWrapper(src.is_ok(), src.get_size(), src.get_value_or_error_type());

        if constexpr (is_move) {
            move_wrapped_object(src.get_value_or_error_type(), dst_res.get_underlying_object_ptr(),
                    src.get_underlying_object_ptr(), max_len);
        } else {
            copy_wrapped_object(src.get_value_or_error_type(), dst_res.get_underlying_object_ptr(),
                    src.get_underlying_object_ptr(), max_len);
        }
    }

    template<typename T, bool is_move, typename SrcPtr = std::conditional_t<is_move, void *, const void *>>
    static void _copy_or_move_type(void *dst, SrcPtr src, size_t max_len) {
        argus_assert(max_len >= sizeof(T));
        if constexpr (is_move) {
            new(dst) T(std::move(*reinterpret_cast<T *>(src)));
        } else {
            new(dst) T(*reinterpret_cast<const T *>(src));
        }
    }

    template<typename T>
    static void _destruct_type(void *ptr) {
        reinterpret_cast<T *>(ptr)->~T();
    }

    template<bool is_move, typename SrcPtr = std::conditional<is_move, void *, const void *>>
    static void _copy_or_move_wrapped_object(const ObjectType &obj_type, void *dst, SrcPtr src, size_t size) {
        if (obj_type.type != IntegralType::String) {
            affirm_precond(size >= obj_type.size, "Can't copy wrapped object: dest size is too small");
        }

        switch (obj_type.type) {
            case IntegralType::Void: {
                // no-op

                break;
            }
            case IntegralType::Struct: {
                // for complex value types we indirectly use the copy/move
                // constructors

                argus_assert(obj_type.type_id.has_value());

                auto bound_type = get_bound_type(obj_type.type_id.value())
                        .expect("Tried to copy/move wrapped object with unbound struct type");
                if constexpr (is_move) {
                    argus_assert(bound_type.move_ctor != nullptr);
                    bound_type.move_ctor(dst, src);
                } else {
                    argus_assert(bound_type.copy_ctor != nullptr);
                    bound_type.copy_ctor(dst, src);
                }

                break;
            }
            case IntegralType::Pointer: {
                // copy the pointer itself

                memcpy(dst, src, sizeof(void *));

                break;
            }
            case IntegralType::Callback: {
                // we use the copy constructor for callbacks instead of doing a
                // bitwise copy because std::function isn't trivially copyable
                _copy_or_move_type<ProxiedScriptCallback, is_move>(dst, src, size);

                break;
            }
            case IntegralType::Vector: {
                auto &src_blob = *reinterpret_cast<std::conditional_t<is_move, ArrayBlob, const ArrayBlob> *>(src);
                _copy_or_move_array_blob<is_move>(obj_type, dst, src_blob, size);

                break;
            }
            case IntegralType::VectorRef: {
                _copy_or_move_type<VectorWrapper, is_move>(dst, src, size);

                break;
            }
            case IntegralType::Result: {
                auto &src_res = *reinterpret_cast<std::conditional_t<is_move, ResultWrapper, const ResultWrapper> *>(src);
                _copy_or_move_result_wrapper<is_move>(dst, src_res, size);

                break;
            }
            default: {
                // for everything else we bitwise-copy the value
                // note that std::type_index is trivially copyable
                memcpy(dst, src, size);

                break;
            }
        }
    }

    void copy_wrapped_object(const ObjectType &obj_type, void *dst, const void *src, size_t size) {
        _copy_or_move_wrapped_object<false>(obj_type, dst, src, size);
    }

    void move_wrapped_object(const ObjectType &obj_type, void *dst, void *src, size_t size) {
        _copy_or_move_wrapped_object<true>(obj_type, dst, src, size);
    }

    void destruct_wrapped_object(const ObjectType &obj_type, void *ptr) {
        switch (obj_type.type) {
            case IntegralType::Void: {
                // no-op

                break;
            }
            case IntegralType::Struct: {
                // for complex value types we indirectly use the copy/move
                // constructors

                argus_assert(obj_type.type_id.has_value());

                auto bound_type = get_bound_type(obj_type.type_id.value())
                        .expect("Tried to destruct wrapped object with unbound struct type");
                argus_assert(bound_type.dtor != nullptr);
                bound_type.dtor(ptr);

                break;
            }
            case IntegralType::Callback: {
                _destruct_type<ProxiedScriptCallback>(ptr);

                break;
            }
            case IntegralType::Vector: {
                // ArrayBlob destructor implicitly destructs its elements
                _destruct_type<ArrayBlob>(ptr);

                break;
            }
            case IntegralType::VectorRef: {
                _destruct_type<VectorWrapper>(ptr);

                break;
            }
            default: {
                // no-op
            }
        }
    }
}

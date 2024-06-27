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

#include <stdexcept>
#include <string>

#include <cstring>

namespace argus {
    Result<ObjectWrapper, ReflectiveArgumentsError> create_object_wrapper(const ObjectType &type, const void *ptr, size_t size) {
        ObjectWrapper wrapper(type, size);

        switch (type.type) {
            case IntegralType::Vector:
                crash("create_object_wrapper called for Vector type (use vector-specific function instead)");
            case IntegralType::Pointer: {
                // for pointer types we copy the pointer itself
                memcpy(wrapper.get_ptr(), &ptr, wrapper.buffer_size);
                break;
            }
            case IntegralType::Struct: {
                // for complex value types we indirectly use the copy constructor
                assert(type.type_index.has_value());

                auto bound_type = get_bound_type(type.type_index.value())
                        .expect("Tried to create ObjectWrapper with unbound struct type");
                assert(bound_type.copy_ctor != nullptr);
                bound_type.copy_ctor(wrapper.get_ptr(), ptr);
                wrapper.copy_ctor = bound_type.copy_ctor;
                wrapper.move_ctor = bound_type.move_ctor;
                wrapper.dtor = bound_type.dtor;

                break;
            }
            case IntegralType::Callback: {
                // we use the copy constructor instead of doing a bitwise copy because
                // std::function isn't trivially copyable
                new(wrapper.get_ptr()) ProxiedScriptCallback(*reinterpret_cast<const ProxiedScriptCallback *>(ptr));
                wrapper.copy_ctor = [](void *dst, const void *src) {
                    new(dst) ProxiedScriptCallback(*reinterpret_cast<const ProxiedScriptCallback *>(src));
                };
                wrapper.move_ctor = [](void *dst, void *src) {
                    new(dst) ProxiedScriptCallback(std::move(*reinterpret_cast<ProxiedScriptCallback *>(src)));
                };
                wrapper.dtor = [](void *rhs) { reinterpret_cast<ProxiedScriptCallback *>(rhs)->~ProxiedScriptCallback(); };

                break;
            }
            default: {
                // for everything else we bitwise-copy the value
                // note that std::type_index is trivially copyable
                memcpy(wrapper.get_ptr(), ptr, wrapper.buffer_size);

                break;
            }
        }

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_object_wrapper(const ObjectType &type, const void *ptr) {
        affirm_precond(type.type != IntegralType::String, "Cannot create object wrapper for string-typed value - "
                                                          "string-specific overload must be used");

        return create_object_wrapper(type, ptr, type.size);
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_int_object_wrapper(const ObjectType &type, int64_t val) {
        ObjectWrapper wrapper(type, type.size);
        assert(wrapper.buffer_size >= type.size);

        if (type.type == IntegralType::Enum) {
            assert(type.type_name.has_value());
            auto enum_def = get_bound_enum(type.type_name.value())
                    .expect("Tried to create ObjectWrapper with unbound enum type");
            auto enum_val_it = enum_def.all_ordinals.find(*reinterpret_cast<uint64_t *>(&val));
            if (enum_val_it == enum_def.all_ordinals.cend()) {
                return err<ObjectWrapper, ReflectiveArgumentsError>("Unknown ordinal " + std::to_string(val)
                        + " for enum type " + enum_def.name);
            }
        }

        switch (type.size) {
            case 1: {
                assert(wrapper.buffer_size >= sizeof(int8_t));
                *reinterpret_cast<int8_t *>(wrapper.get_ptr()) = int8_t(val);
                break;
            }
            case 2: {
                assert(wrapper.buffer_size >= sizeof(int16_t));
                *reinterpret_cast<int16_t *>(wrapper.get_ptr()) = int16_t(val);
                break;
            }
            case 4: {
                assert(wrapper.buffer_size >= sizeof(int32_t));
                *reinterpret_cast<int32_t *>(wrapper.get_ptr()) = int32_t(val);
                break;
            }
            case 8: {
                assert(wrapper.buffer_size >= sizeof(int64_t));
                *reinterpret_cast<int64_t *>(wrapper.get_ptr()) = val;
                break;
            }
            default:
                assert(false); // should have been caught during binding
        }

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_float_object_wrapper(const ObjectType &type, double val) {
        ObjectWrapper wrapper(type, type.size);

        switch (type.size) {
            case 4: {
                assert(wrapper.buffer_size >= sizeof(float));
                auto val_f32 = float(val);
                *reinterpret_cast<float *>(wrapper.get_ptr()) = val_f32;
                break;
            }
            case 8: {
                assert(wrapper.buffer_size >= sizeof(double));
                *reinterpret_cast<double *>(wrapper.get_ptr()) = val;
                break;
            }
            default:
                assert(false); // should have been caught during binding
        }

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> create_bool_object_wrapper(const ObjectType &type, bool val) {
        assert(type.size >= sizeof(bool));

        ObjectWrapper wrapper(type, type.size);

        *reinterpret_cast<bool *>(wrapper.get_ptr()) = val;

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

    Result<ObjectWrapper, ReflectiveArgumentsError> create_callback_object_wrapper(const ObjectType &type, const ProxiedScriptCallback &fn) {
        affirm_precond(type.type == IntegralType::Callback,
                "Cannot create object wrapper "
                "(callback-specific overload called for non-callback-typed value)");

        return create_object_wrapper(type, &fn, sizeof(fn));
    }

    static void _validate_vec_obj_type(ObjectType vec_type) {
        ObjectType &el_type = *vec_type.element_type.value();

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

        ObjectType &el_type = *vec_type.element_type.value();

        bool is_trivially_copyable = el_type.type != IntegralType::String
                && !(el_type.type == IntegralType::Struct
                        && get_bound_type(el_type.type_index.value())
                                .expect("Tried to create ObjectWrapper with unbound type").copy_ctor != nullptr);

        size_t el_size = el_type.size;
        if (el_type.type == IntegralType::String) {
            el_size = sizeof(std::string);
        }

        size_t blob_size = sizeof(ArrayBlob) + el_size * count;

        ObjectWrapper wrapper(vec_type, blob_size);
        ArrayBlob &blob = *new(wrapper.get_ptr()) ArrayBlob(el_size, count,
                el_type.type == IntegralType::String
                        ? +[](void *ptr) { reinterpret_cast<std::string *>(ptr)->~basic_string(); }
                        : nullptr);

        if (is_trivially_copyable) {
            // can just copy the whole thing in one go and avoid looping
            memcpy(blob[0], data, el_size * count);
        } else {
            assert(count < SIZE_MAX);

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
                assert(el_type.type == IntegralType::Struct);

                auto bound_type_res = get_bound_type(el_type.type_index.value());
                const BoundTypeDef &bound_type = bound_type_res
                        .expect("Tried to create ObjectWrapper with unbound struct type");
                assert(bound_type.copy_ctor != nullptr);

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

        new(wrapper.get_ptr()) VectorWrapper(std::move(vec));

        return ok<ObjectWrapper, ReflectiveArgumentsError>(std::move(wrapper));
    }
}

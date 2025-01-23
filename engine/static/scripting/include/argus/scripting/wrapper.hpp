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

#include <string>
#include <type_traits>

namespace argus {
    struct BindingError;

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
            const ProxiedScriptCallback &fn
    );

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

    void copy_wrapped_object(const ObjectType &obj_type, void *dst, const void *src, size_t max_len);

    void move_wrapped_object(const ObjectType &obj_type, void *dst, void *src, size_t size);

    void destruct_wrapped_object(const ObjectType &obj_type, void *ptr);
}

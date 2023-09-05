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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/exception.hpp"
#include "argus/scripting/types.hpp"
#include "argus/scripting/util.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <new>
#include <string>
#include <vector>

#include <cstdlib>
#include <cstring>

namespace argus {
    static const BoundFunctionDef &_get_native_function(FunctionType fn_type,
            const std::string &type_name, const std::string &fn_name) {
        switch (fn_type) {
            case MemberInstance:
            case MemberStatic: {
                auto type_it = g_bound_types.find(type_name);
                if (type_it == g_bound_types.cend()) {
                    throw TypeNotBoundException(type_name);
                }

                const auto &fn_map = fn_type == MemberInstance
                        ? type_it->second.instance_functions
                        : type_it->second.static_functions;

                auto fn_it = fn_map.find(fn_name);
                if (fn_it == fn_map.cend()) {
                    throw SymbolNotBoundException(get_qualified_function_name(fn_type, type_name, fn_name));
                }
                return fn_it->second;
            }
            case Global: {
                auto it = g_bound_global_fns.find(fn_name);
                if (it == g_bound_global_fns.cend()) {
                    throw SymbolNotBoundException(fn_name);
                }
                return it->second;
            }
            default:
                Logger::default_logger().fatal("Unknown function type ordinal %d", fn_type);
        }
    }

    static const BoundFieldDef &_get_native_field(const std::string &type_name, const std::string &field_name) {
        auto type_it = g_bound_types.find(type_name);
        if (type_it == g_bound_types.cend()) {
            throw TypeNotBoundException(type_name);
        }

        const auto &field_map = type_it->second.fields;

        auto field_it = field_map.find(field_name);
        if (field_it == field_map.cend()) {
            throw SymbolNotBoundException(get_qualified_field_name(type_name, field_name));
        }
        return field_it->second;
    }

    const BoundTypeDef &get_bound_type(const std::string &type_name) {
        auto it = g_bound_types.find(type_name);
        if (it == g_bound_types.cend()) {
            throw std::invalid_argument("Type name" + std::string(type_name)
                                        + " is not bound (check binding order and ensure bind_type"
                                          " is called after creating type definition)");
        }
        return it->second;
    }

    const BoundTypeDef &get_bound_type(const std::type_info &type_info) {
        return get_bound_type(std::type_index(type_info));
    }

    template <typename T>
    T &_get_bound_type(const std::type_index type_index) {
        auto index_it = g_bound_type_indices.find(std::type_index(type_index));
        if (index_it == g_bound_type_indices.cend()) {
            throw std::invalid_argument("Type " + std::string(type_index.name())
                    + " is not bound (check binding order and ensure bind_type"
                      " is called after creating type definition)");
        }
        auto type_it = g_bound_types.find(index_it->second);
        assert(type_it != g_bound_types.cend());
        return const_cast<T &>(type_it->second);
    }

    const BoundTypeDef &get_bound_type(std::type_index type_index) {
        return _get_bound_type<const BoundTypeDef>(type_index);
    }

    const BoundEnumDef &get_bound_enum(const std::string &enum_name) {
        auto it = g_bound_enums.find(enum_name);
        if (it == g_bound_enums.cend()) {
            throw std::invalid_argument("Enum name" + std::string(enum_name)
                                        + " is not bound (check binding order and ensure bind_type"
                                          " is called after creating type definition)");
        }
        return it->second;
    }

    const BoundEnumDef &get_bound_enum(const std::type_info &enum_type_info) {
        return get_bound_enum(std::type_index(enum_type_info));
    }

    template <typename T>
    T &_get_bound_enum(std::type_index enum_type_index) {
        auto index_it = g_bound_enum_indices.find(std::type_index(enum_type_index));
        if (index_it == g_bound_enum_indices.cend()) {
            throw std::invalid_argument("Enum " + std::string(enum_type_index.name())
                                        + " is not bound (check binding order and ensure bind_type"
                                          " is called after creating type definition)");
        }
        auto enum_it = g_bound_enums.find(index_it->second);
        assert(enum_it != g_bound_enums.cend());
        return const_cast<T &>(enum_it->second);
    }

    const BoundEnumDef &get_bound_enum(std::type_index enum_type_index) {
        return _get_bound_enum<const BoundEnumDef>(enum_type_index);
    }

    const BoundFunctionDef &get_native_global_function(const std::string &name) {
        return _get_native_function(FunctionType::Global, "", name);
    }

    const BoundFunctionDef &get_native_member_instance_function(const std::string &type_name,
            const std::string &fn_name) {
        return _get_native_function(FunctionType::MemberInstance, type_name, fn_name);
    }

    const BoundFunctionDef &get_native_member_static_function(const std::string &type_name,
            const std::string &fn_name) {
        return _get_native_function(FunctionType::MemberStatic, type_name, fn_name);
    }

    const BoundFieldDef &get_native_member_field(const std::string &type_name, const std::string &field_name) {
        return _get_native_field(type_name, field_name);
    }

    ObjectWrapper invoke_native_function(const BoundFunctionDef &def, const std::vector<ObjectWrapper> &params) {
        auto expected_param_count = def.params.size();
        if (def.type == FunctionType::MemberInstance) {
            expected_param_count += 1;
        }

        if (params.size() < expected_param_count) {
            throw ReflectiveArgumentsException("Too few arguments provided");
        } else if (params.size() > expected_param_count) {
            throw ReflectiveArgumentsException("Too many arguments provided");
        }

        assert(params.size() == expected_param_count);

        return def.handle(params);
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr, size_t size) {
        ObjectWrapper wrapper(type, size);

        switch (type.type) {
            case Pointer: {
                // for pointer types we copy the pointer itself
                memcpy(wrapper.get_ptr(), &ptr, wrapper.buffer_size);
                break;
            }
            case Struct: {
                // for complex value types we indirectly use the copy constructor
                assert(type.type_index.has_value());

                auto bound_type = get_bound_type(type.type_index.value());
                assert(bound_type.copy_ctor.has_value());
                bound_type.copy_ctor.value()(wrapper.get_ptr(), ptr);
                wrapper.copy_ctor = bound_type.copy_ctor;
                wrapper.move_ctor = bound_type.move_ctor;
                wrapper.dtor = bound_type.dtor;

                break;
            }
            case Callback: {
                new(wrapper.get_ptr()) ProxiedFunction(*reinterpret_cast<const ProxiedFunction *>(ptr));
                wrapper.copy_ctor = [](void *dst, const void *src) {
                    return new(dst) ProxiedFunction(*reinterpret_cast<const ProxiedFunction *>(src));
                };
                wrapper.move_ctor = [](void *dst, void *src) {
                    return new(dst) ProxiedFunction(std::move(*reinterpret_cast<ProxiedFunction *>(src)));
                };
                wrapper.dtor = [](void *rhs) { reinterpret_cast<ProxiedFunction *>(rhs)->~ProxiedFunction(); };

                break;
            }
            default: {
                // for everything else we bitwise-copy the value
                // note that std::type_index is trivially copyable
                memcpy(wrapper.get_ptr(), ptr, wrapper.buffer_size);

                break;
            }
        }

        return wrapper;
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr) {
        affirm_precond(type.type != IntegralType::String, "Cannot create object wrapper for string-typed value - "
                "string-specific overload must be used");
        affirm_precond(type.type != IntegralType::Callback, "Cannot create object wrapper for callback-typed "
                "value - callback-specific overload must be used");

        return create_object_wrapper(type, ptr, type.size);
    }

    ObjectWrapper create_string_object_wrapper(const ObjectType &type, const std::string &str) {
        affirm_precond(type.type == IntegralType::String, "Cannot create object wrapper (string-specific overload "
                "called for non-string-typed value");

        return create_object_wrapper(type, str.c_str(), str.length() + 1);
    }

    ObjectWrapper create_callback_object_wrapper(const ObjectType &type, const ProxiedFunction &fn) {
        affirm_precond(type.type == IntegralType::Callback,
                "Cannot create object wrapper "
                "(callback-specific overload called for non-callback-typed value)");

        ObjectWrapper wrapper(type, sizeof(fn));

        // we use the copy constructor instead of doing a bitwise copy because
        // std::function isn't trivially copyable
        new(wrapper.get_ptr()) ProxiedFunction(fn);

        return create_object_wrapper(type, &fn, sizeof(fn));
    }

    ObjectWrapper create_vector_object_wrapper(const ObjectType &vec_type, const void *data, size_t count,
            bool is_trivially_copyable) {
        affirm_precond(vec_type.type == IntegralType::Vector,
                "Cannot create object wrapper (vector-specific overload called for non-vector-typed value)");

        ObjectType &el_type = *vec_type.element_type.value().get();

        if (el_type.type == IntegralType::Void) {
            throw std::invalid_argument("Vectors of void are not supported");
        } else if (el_type.type == IntegralType::Callback) {
            throw std::invalid_argument("Vectors of callbacks are not supported");
        } else if (el_type.type == IntegralType::Type) {
            throw std::invalid_argument("Vectors of types are not supported");
        } else if (el_type.type == IntegralType::Vector) {
            throw std::invalid_argument("Vectors of vectors are not supported");
        }

        size_t el_size = el_type.size;
        if (el_type.type == IntegralType::String) {
            el_size = sizeof(std::string);
        }

        size_t blob_size = sizeof(ArrayBlob) + el_size * count;

        ObjectWrapper wrapper(vec_type, blob_size);

        if (is_trivially_copyable) {
            // can just copy the whole thing in one go and avoid looping
            memcpy(wrapper.get_ptr(), data, blob_size);
        } else {
            ArrayBlob &blob = *reinterpret_cast<ArrayBlob *>(wrapper.get_ptr());

            affirm_precond(count < SIZE_MAX, "Too many vector elements");

            if (el_type.type == IntegralType::String) {
                // strings need to be handled specially because they're the only
                // non-struct type allowed in a vector that isn't trivially
                // copyable
                for (size_t i = 0; i < count; i++) {
                    auto src_ptr = reinterpret_cast<std::string *>(
                            reinterpret_cast<uintptr_t>(data) + uintptr_t(el_size));
                    void *dst_ptr = blob[i];
                    new(dst_ptr) std::string(*src_ptr);
                }
            } else {
                assert(el_type.type == IntegralType::Struct);

                const BoundTypeDef &bound_type = get_bound_type(el_type.type_index.value());
                assert(bound_type.copy_ctor.has_value());

                for (size_t i = 0; i < count; i++) {
                    void *src_ptr = reinterpret_cast<void *>(
                            reinterpret_cast<uintptr_t>(data) + uintptr_t(el_size));
                    void *dst_ptr = blob[i];

                    bound_type.copy_ctor.value()(dst_ptr, src_ptr);
                }
            }
        }

        return wrapper;
    }

    BoundTypeDef create_type_def(const std::string &name, size_t size, std::type_index type_index,
            std::optional<std::function<void(void *dst, const void *src)>> copy_ctor,
            std::optional<std::function<void(void *dst, void *src)>> move_ctor,
            std::optional<std::function<void(void *obj)>> dtor) {
        BoundTypeDef def {
                name,
                size,
                type_index,
                std::move(copy_ctor),
                std::move(move_ctor),
                std::move(dtor),
                {},
                {},
                {}
        };
        return def;
    }

    void add_member_instance_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (type_def.instance_functions.find(fn_def.name) != type_def.instance_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::MemberInstance, type_def.name, fn_def.name);
            throw BindingException(qual_name, "Instance function with same name is already bound");
        }

        type_def.instance_functions.insert({ fn_def.name, fn_def });
    }

    void bind_member_instance_function(std::type_index type_index, const BoundFunctionDef &fn_def) {
        auto &type_def = _get_bound_type<BoundTypeDef>(type_index);
        add_member_instance_function(type_def, fn_def);
    }

    void add_member_static_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (type_def.static_functions.find(fn_def.name) != type_def.static_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::MemberStatic, type_def.name, fn_def.name);
            throw BindingException(qual_name, "Static function with same name is already bound");
        }

        type_def.static_functions.insert({ fn_def.name, fn_def });
    }

    void bind_member_static_function(std::type_index type_index, const BoundFunctionDef &fn_def) {
        auto &type_def = _get_bound_type<BoundTypeDef>(type_index);
        add_member_static_function(type_def, fn_def);
    }

    void add_member_field(BoundTypeDef &type_def, const BoundFieldDef &field_def) {
        if (type_def.fields.find(field_def.m_name) != type_def.fields.cend()) {
            auto qual_name = get_qualified_field_name(type_def.name, field_def.m_name);
            throw BindingException(qual_name, "Instance function with same name is already bound");
        }

        type_def.fields.insert({ field_def.m_name, field_def });
    }

    void bind_member_field(std::type_index type_index, const BoundFieldDef &field_def) {
        auto &type_def = _get_bound_type<BoundTypeDef>(type_index);
        add_member_field(type_def, field_def);
    }

    BoundEnumDef create_enum_def(const std::string &name, size_t width, std::type_index type_index) {
        return { name, width, type_index, {}, {} };
    }

    void add_enum_value(BoundEnumDef &def, const std::string &name, uint64_t value) {
        if (def.values.find(name) != def.values.cend()) {
            throw BindingException(def.name + "::" + name, "Enum value with same name is already bound");
        }

        if (def.all_ordinals.find(value) != def.all_ordinals.cend()) {
            throw BindingException(def.name + "::" + name, "Enum value with same ordinal is already bound");
        }

        def.values.insert({ name, value });
        def.all_ordinals.insert(value);
    }

    void bind_enum_value(std::type_index enum_type, const std::string &name, uint64_t value) {
        auto &enum_def = _get_bound_enum<BoundEnumDef>(enum_type);
        add_enum_value(enum_def, name, value);
    }
}

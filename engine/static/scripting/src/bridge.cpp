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

#include <string>
#include <vector>

#include <cstdlib>
#include <cstring>
#include <cassert>

namespace argus {
    static const BoundFunctionDef &_get_native_function(FunctionType fn_type,
            const std::string &type_name, const std::string &fn_name) {
        switch (fn_type) {
            case FunctionType::MemberInstance:
            case FunctionType::MemberStatic: {
                auto type_it = g_bound_types.find(type_name);
                if (type_it == g_bound_types.cend()) {
                    throw TypeNotBoundException(type_name);
                }

                const auto &fn_map = fn_type == FunctionType::MemberInstance
                        ? type_it->second.instance_functions
                        : type_it->second.static_functions;

                auto fn_it = fn_map.find(fn_name);
                if (fn_it == fn_map.cend()) {
                    throw FunctionNotBoundException(get_qualified_function_name(fn_type, type_name, fn_name));
                }
                return fn_it->second;
            }
            case FunctionType::Global: {
                auto it = g_bound_global_fns.find(fn_name);
                if (it == g_bound_global_fns.cend()) {
                    throw FunctionNotBoundException(fn_name);
                }
                return it->second;
            }
            default:
                Logger::default_logger().fatal("Unknown function type ordinal %d", fn_type);
        }
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

    const BoundTypeDef &get_bound_type(const std::type_index &type_index) {
        auto index_it = g_bound_type_indices.find(std::type_index(type_index));
        if (index_it == g_bound_type_indices.cend()) {
            throw std::invalid_argument("Type " + std::string(type_index.name())
                    + " is not bound (check binding order and ensure bind_type"
                      " is called after creating type definition)");
        }
        auto type_it = g_bound_types.find(index_it->second);
        assert(type_it != g_bound_types.cend());
        return type_it->second;
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

    const BoundEnumDef &get_bound_enum(const std::type_index &enum_type_index) {
        auto index_it = g_bound_enum_indices.find(std::type_index(enum_type_index));
        if (index_it == g_bound_enum_indices.cend()) {
            throw std::invalid_argument("Enum " + std::string(enum_type_index.name())
                                        + " is not bound (check binding order and ensure bind_type"
                                          " is called after creating type definition)");
        }
        auto enum_it = g_bound_enums.find(index_it->second);
        assert(enum_it != g_bound_enums.cend());
        return enum_it->second;
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

    ObjectWrapper invoke_native_function(const BoundFunctionDef &def, const std::vector<ObjectWrapper> &params) {
        auto expected_param_count = def.params.size();
        if (def.type == FunctionType::MemberInstance) {
            expected_param_count += 1;
        }

        if (params.size() < expected_param_count) {
            throw ReflectiveArgumentsException("Too few arguments provided");
        } else if (params.size() >= expected_param_count) {
            throw ReflectiveArgumentsException("Too many arguments provided");
        }

        assert(params.size() == expected_param_count);

        return def.handle(params);
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, const void *ptr, size_t size) {
        ObjectWrapper wrapper{};
        assert(type.type == IntegralType::String || type.size == size);
        wrapper.type = type;

        // for pointer types we copy the pointer itself, and for everything else we copy the value
        const void *copy_src = type.type == IntegralType::Pointer ? &ptr : ptr;
        // override size for pointer type since we're only copying the pointer
        size_t copy_size = type.type == IntegralType::Pointer
            ? sizeof(void *)
            : type.type == IntegralType::String
                ? size
                : type.size;

        if (copy_size <= sizeof(wrapper.value)) {
            // can store directly in ObjectWrapper struct
            memcpy(wrapper.value, copy_src, copy_size);
            wrapper.is_on_heap = false;
        } else {
            // need to alloc on heap
            wrapper.heap_ptr = malloc(copy_size);
            memcpy(wrapper.heap_ptr, copy_src, copy_size);
            wrapper.is_on_heap = true;
        }

        return wrapper;
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, void *ptr) {
        if (type.type == IntegralType::String) {
            throw std::runtime_error("Cannot create object wrapper for string-typed value - overload must be used");
        }

        return create_object_wrapper(type, ptr, type.size);
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, const std::string &str) {
        if (type.type != IntegralType::String) {
            throw std::runtime_error("Cannot create object wrapper (string-specific overload called for"
                                     " non-string-typed value");
        }

        auto wrapper = create_object_wrapper(type, str.c_str(), str.length() + 1);
        return wrapper;
    }

    void cleanup_object_wrapper(ObjectWrapper &wrapper) {
        if (wrapper.is_on_heap) {
            free(wrapper.heap_ptr);
        }
    }

    void cleanup_object_wrappers(std::vector<ObjectWrapper> &wrappers) {
        for (auto &wrapper : wrappers) {
            cleanup_object_wrapper(wrapper);
        }
    }

    void add_member_instance_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (type_def.instance_functions.find(fn_def.name) != type_def.instance_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::MemberInstance, type_def.name, fn_def.name);
            throw BindingException(qual_name, "Instance function with same name is already bound");
        }

        type_def.instance_functions.insert({ fn_def.name, fn_def });
    }

    void add_member_static_function(BoundTypeDef &type_def, const BoundFunctionDef &fn_def) {
        if (type_def.static_functions.find(fn_def.name) != type_def.static_functions.cend()) {
            auto qual_name = get_qualified_function_name(FunctionType::MemberStatic, type_def.name, fn_def.name);
            throw BindingException(qual_name, "Static function with same name is already bound");
        }

        type_def.static_functions.insert({ fn_def.name, fn_def });
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
}

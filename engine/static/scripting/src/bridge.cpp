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

#include "argus/lowlevel/logging.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/exception.hpp"
#include "argus/scripting/types.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "argus/scripting/util.hpp"

#include <string>
#include <vector>

#include <cstdlib>
#include <cstring>
#include <cassert>

namespace argus {
    void *scripting_copy_value(void *src, size_t size) {
        void *dst = malloc(size);
        memcpy(dst, src, size);
        return dst;
    }

    void scripting_free_value(void *buf) {
        free(buf);
    }

    static const BoundFunctionDef &_get_native_function(FunctionType fn_type, const std::string &fn_name,
            const std::string &type_name) {
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

    const BoundFunctionDef &get_native_global_function(const std::string &name) {
        return _get_native_function(FunctionType::Global, name, "");
    }

    const BoundFunctionDef &get_native_member_instance_function(const std::string &fn_name, const std::string &type_name) {
        return _get_native_function(FunctionType::MemberInstance, fn_name, type_name);
    }

    const BoundFunctionDef &get_native_member_static_function(const std::string &fn_name, const std::string &type_name) {
        return _get_native_function(FunctionType::MemberStatic, fn_name, type_name);
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

    static ObjectWrapper _create_object_wrapper(const void *ptr, size_t size) {
        ObjectWrapper wrapper{};
        if (size <= sizeof(wrapper.value)) {
            // can store directly in ObjectWrapper struct
            memcpy(wrapper.value, ptr, size);
            wrapper.is_on_heap = false;
        } else {
            // need to alloc on heap
            wrapper.heap_ptr = malloc(size);
            memcpy(wrapper.heap_ptr, ptr, size);
            wrapper.is_on_heap = true;
        }

        return wrapper;
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, void *ptr) {
        if (type.type == IntegralType::String) {
            throw std::runtime_error("Cannot create object wrapper for string-typed value - overload must be used");
        }

        return _create_object_wrapper(ptr, type.size);
    }

    ObjectWrapper create_object_wrapper(const ObjectType &type, const std::string &str) {
        if (type.type != IntegralType::String) {
            throw std::runtime_error("Cannot create object wrapper (string-specific overload called for"
                                     " non-string-typed value");
        }

        return _create_object_wrapper(str.c_str(), str.length() + 1);
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
}

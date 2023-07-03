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

#include "argus/lowlevel/functional.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"

#include "argus/scripting/bridge.hpp"
#include "argus/scripting/types.hpp"
#include "internal/scripting/module_scripting.hpp"

#include <functional>
#include <type_traits>
#include <vector>

#include <cstdlib>
#include <cstring>

namespace argus {
    static constexpr const char TYPE_NAME_SEPARATOR[] = "#";

    void *copy_value(void *src, size_t size) {
        void *dst = malloc(size);
        memcpy(dst, src, size);
        return dst;
    }

    void free_value(void *buf) {
        free(buf);
    }

    static ObjectProxy _invoke_native_function(const std::string &name, const std::vector<ObjectProxy> &params) {
        auto it = g_registered_fn_handles.find(name);
        if (it == g_registered_fn_handles.cend()) {
            //TODO: throw exception that we can bubble up to the language plugin
            return {};
        }

        return it->second(params);
    }

    ObjectProxy invoke_native_function_global(const std::string &name, const std::vector<ObjectProxy> &params) {
        return _invoke_native_function(name, params);
    }

    ObjectProxy invoke_native_function_instance(const std::string &name, const std::string &type_name,
            ObjectProxy instance, const std::vector<ObjectProxy> &params) {
        auto qualified_name = type_name + TYPE_NAME_SEPARATOR + name;
        std::vector<ObjectProxy> new_params(params.size() + 1);
        new_params[0] = instance;
        std::copy(params.cbegin(), params.cend(), new_params.begin());

        return _invoke_native_function(qualified_name, new_params);
    }

    static ObjectProxy _create_object_proxy(const void *ptr, size_t size) {
        ObjectProxy proxy{};
        if (size <= sizeof(proxy.value)) {
            // can store directly in ObjectProxy struct
            memcpy(proxy.value, ptr, size);
            proxy.is_on_heap = false;
        } else {
            // need to alloc on heap
            proxy.heap_ptr = malloc(size);
            memcpy(proxy.heap_ptr, ptr, size);
            proxy.is_on_heap = true;
        }

        return proxy;
    }

    ObjectProxy create_object_proxy(const ObjectType &type, void *ptr) {
        if (type.type == IntegralType::String) {
            throw std::runtime_error("Cannot create object proxy for string-typed value - overload must be used");
        }

        return _create_object_proxy(ptr, type.size);
    }

    ObjectProxy create_object_proxy(const ObjectType &type, const std::string &str) {
        if (type.type != IntegralType::String) {
            throw std::runtime_error("Cannot create object proxy (string-specific overload called for"
                                     " non-string-typed value");
        }

        return _create_object_proxy(str.c_str(), str.length() + 1);
    }
}

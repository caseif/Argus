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

#include "argus/lowlevel/macros.hpp"

#include "argus/scripting/bridge.hpp"

#include <functional>
#include <type_traits>
#include <vector>

#include <cstring>

namespace argus {
    template <typename T>
    static T &_unwrap_object_proxy(ObjectProxy proxy) {
        if (std::is_reference_v<T>) {
            return *reinterpret_cast<T *>(proxy.ptr);
        } else if (std::is_pointer_v<T>) {
            return reinterpret_cast<T *>(proxy.ptr);
        } else {
            T val;
            memcpy(val, proxy.ptr, sizeof(T));
            return val;
        }
    }

    template <typename... Args>
    static std::tuple<Args...> _convert_proxies_to_tuple(std::vector<ObjectProxy> &params) {
        std::tuple<Args...> args_tuple;
        std::apply([&](auto&&... args) {
            size_t i = 0;
            auto assign_param = [&](auto &arg) {
                arg = _unwrap_object_proxy<decltype(arg)>(params[i++]);
            };
            (assign_param(args), ...);
        }, args_tuple);
        return args_tuple;
    }

    template <typename T>
    static ObjectProxy _convert_native_to_proxy(T val) {
        if (std::is_reference_v<T>) {
            return { &val };
        } else if (std::is_pointer_v<T>) {
            return { val };
        } else {
            void *buf = malloc(sizeof(T));
            memcpy(buf, val, sizeof(T));
            return { buf };
        }
    }

    template <typename ReturnType, typename... Args>
    static ObjectProxy _invoke_fn_global(ReturnType(*fn)(Args...),
            std::vector<ObjectProxy> params) {
        std::tuple<Args...> native_args = _convert_proxies_to_tuple<Args...>(params);
        return _convert_native_to_proxy(std::apply(fn, native_args));
    }

    template <typename ReturnType, typename InstanceType, typename... Args>
    static ObjectProxy _invoke_fn_instance(ReturnType(InstanceType::*fn)(Args...),
            ObjectProxy instance, std::vector<ObjectProxy> params) {
        std::tuple<Args...> native_args = _convert_proxies_to_tuple<Args...>(params);
        return _convert_native_to_proxy(std::apply([&](auto&&... args) { std::invoke(fn, instance, args...); },
                native_args));
    }

    ObjectProxy invoke_native_function_global(const std::string &name, std::vector<ObjectProxy> params) {
        UNUSED(name);
        UNUSED(params);
        //TODO
        return {};
    }

    ObjectProxy invoke_native_function_instance(const std::string &name, const std::string &type_name,
            ObjectProxy instance, std::vector<ObjectProxy> params) {
        UNUSED(name);
        UNUSED(type_name);
        UNUSED(instance);
        UNUSED(params);
        //TODO
        return {};
    }
}

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

#include "argus/lowlevel/functional.hpp"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace argus {
    struct ObjectProxy {
        void *ptr;
    };

    class InvocationException : public std::exception {
      private:
        const std::string msg;

      public:
        InvocationException(std::string msg) : msg(std::move(msg)) {
        }

        /**
         * \copydoc std::exception::what()
         *
         * \return The exception message.
         */
        [[nodiscard]] const char *what(void) const noexcept override {
            return msg.c_str();
        }
    };

    template <typename FuncType, typename... Args,
            typename ReturnType = typename function_traits<FuncType>::return_type>
    ReturnType invoke_function(FuncType fn, std::vector<ObjectProxy> params) {
        using ClassType = typename function_traits<FuncType>::class_type;
        using ArgsTuple = typename function_traits<FuncType>::argument_types;

        if (params.size() != std::tuple_size<ArgsTuple>::value
                + (std::is_member_function_pointer_v<FuncType> ? 1 : 0)) {
            throw InvocationException("Wrong parameter count");
        }

        ArgsTuple args;
        auto it = params.begin() + (std::is_member_function_pointer_v<FuncType> ? 0 : 1);
        std::apply([&](auto&... tuple_element) {
            ((tuple_element = *reinterpret_cast<typename std::remove_reference<decltype(tuple_element)>::type*>
                    ((it++)->ptr)), ...);
        }, args);

        if constexpr (!std::is_void_v<ClassType>) {
            ++it;
            ClassType *instance = reinterpret_cast<ClassType*>(std::get<0>(args));
            return std::apply([=](auto&&... args){ return (instance->*fn)(std::forward<decltype(args)>(args)...); },
                    args);
        } else {
            return std::apply(fn, args);
        }
    }

    ObjectProxy invoke_native_function_global(const std::string &name, const std::vector<ObjectProxy> &params);

    ObjectProxy invoke_native_function_instance(const std::string &name, const std::string &type_name,
            ObjectProxy instance, const std::vector<ObjectProxy> &params);
}

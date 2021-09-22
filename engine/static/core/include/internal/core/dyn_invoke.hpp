/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace argus {
    /**
     * \brief Registers a function provided by a module for dynamic invocation
     *        from base engine code.
     *
     * \param fn_name The name of the function.
     * \param addr A pointer to the function.
     */
    void register_module_fn(const std::string &fn_name, const void *addr);

    const void *lookup_module_fn(const std::string &fn_name);

    template <typename Ret, typename... Args, typename Fn = Ret(*)(Args...)>
    Ret call_module_fn(const std::string &fn_name, Args &&...args) {
        auto fn_ptr = reinterpret_cast<Fn>(const_cast<void*>(lookup_module_fn(fn_name)));

        if (fn_ptr != nullptr) {
            return fn_ptr(std::forward<Args>(args)...);
        } else {
            std::stringstream err_msg;
            err_msg << "Module function ";
            err_msg << fn_name;
            err_msg << " is not registered at this time";
            throw std::runtime_error(err_msg.str());
        }
    }
}

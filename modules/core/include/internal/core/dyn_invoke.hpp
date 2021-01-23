/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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
    void register_module_fn(std::string fn_name, void *addr);

    void *lookup_module_fn(std::string fn_name);

    template <typename Ret, typename... Args>
    Ret call_module_fn(std::string fn_name, Args&&... args) {
        auto fn_ptr = reinterpret_cast<Ret(*)(Args...)>(lookup_module_fn(fn_name));

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
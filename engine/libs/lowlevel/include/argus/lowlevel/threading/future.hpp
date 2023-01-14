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

#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/threading/thread.hpp"

#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>

namespace argus {
    /**
     * \brief Constructs a std::future with the given function as a supplier,
     *        and optionally invoking the given callback upon completion.
     *
     * \param function A function containing a task which will supply the
     *                 returned std::future.
     * \param callback The function to invoke after completion of the task.
     *                 This callback must accept the supplied value and may be
     *                 left absent if unneeded.
     *
     * \tparam Out The type of value provided by the returned std::future.
     *
     * \return The created std::future.
     *
     * \attention The provided functions \em must be thread-safe, as they will
     *            be performed on a new thread.
     */
    template <typename Out>
    std::future<Out> make_future(const std::function<Out(void)> &function, const std::function<void(Out)> &callback) {
        if (!function) {
            throw std::invalid_argument("make_future: Function must be present");
        }

        auto promise_ptr = std::make_shared<std::promise<Out>>();
        std::future<Out> future = promise_ptr->get_future();
        Thread *thread = nullptr;
        thread = &Thread::create(
            [thread, function, callback, promise_ptr](const auto user_data) mutable -> void* {
                UNUSED(user_data);
                try {
                    Out res = function();
                    promise_ptr->set_value_at_thread_exit(res);

                    if (callback != nullptr) {
                        callback(res);
                    }
                } catch (...) {
                    promise_ptr->set_exception(std::make_exception_ptr(std::current_exception()));
                }
                
                thread->destroy();

                return nullptr;
            },
        nullptr);

        return future;
    }

    /**
     * \brief Template specialization for make_future for the `void` type.
     *
     * This is useful when an asynchronous task does not return anything
     * meaningful, but notification of completion is still desired.
     *
     * \param function The function containing the task to be executed.
     * \param callback The callback to be invoked upon completion of the task.
     *
     * \remark This specialization is necessary for technical reasons, as the
     *         `void` type has unique language semantics which require special
     *         handling.
     */
    std::future<void> make_future(const std::function<void(void)> &function, const std::function<void(void)> &callback);
}

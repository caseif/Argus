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

#include "argus/lowlevel/crash.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/result.hpp"

#include <functional>
#include <future>
#include <memory>

namespace argus {
    /**
     * @brief Constructs a std::future with the given function as a supplier
     *        and optionally a callback to be invoked upon completion.
     *
     * @param function A function containing a task which will supply the
     *                 returned std::future.
     * @param callback The function to invoke after completion of the task. This
     *        callback must accept the supplied value and may be left absent if
     *        unneeded.
     *
     * @tparam T The type of value provided by the returned std::future when the
     *         operation succeeds.
     * @tparam E The type of value provided by the returned std::future when the
     *         operation fails.
     *
     * @return The created std::future.
     *
     * @attention The provided functions \em must be thread-safe, as they will
     *            be performed on a new thread.
     */
    template<typename T, typename E>
    std::future<Result<T, E>> make_future(const std::function<Result<T, E>(void)> &function,
            const std::function<void(const Result<T, E> &)> &callback) {
        if (!function) {
            crash_ll("make_future: Function must be present");
        }

        auto promise_ptr = std::make_shared<std::promise<Result<T, E>>>();
        std::future<Result<T, E>> future = promise_ptr->get_future();
        std::thread thread(
                [function, callback, promise_ptr](const auto user_data) -> void * {
                    UNUSED(user_data);
                    Result<T, E> res = function();
                    promise_ptr->set_value_at_thread_exit(res);

                    if (callback != nullptr) {
                        callback(res);
                    }

                    return nullptr;
                },
                nullptr
        );

        return future;
    }

    /**
     * @brief Template specialization for make_future for the `void` type.
     *
     * This is useful when an asynchronous task does not return anything
     * meaningful, but notification of completion is still desired.
     *
     * @param function The function containing the task to be executed.
     * @param callback The callback to be invoked upon completion of the task.
     *
     * @remark This specialization is necessary for technical reasons, as the
     *         `void` type has unique language semantics which require special
     *         handling.
     */
    //std::future<void> make_future(const std::function<void(void)> &function, const std::function<void(void)> &callback);
}

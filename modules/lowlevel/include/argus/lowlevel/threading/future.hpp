/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
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

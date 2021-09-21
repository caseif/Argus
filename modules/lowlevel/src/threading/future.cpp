/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/threading/future.hpp"
#include "argus/lowlevel/threading/thread.hpp"

#include <exception>
#include <functional>
#include <future>
#include <memory>

namespace argus {
    std::future<void> make_future(const std::function<void(void)> &function,
            const std::function<void(void)> &callback) {
        auto promise_ptr = std::make_shared<std::promise<void>>();
        std::future<void> future = promise_ptr->get_future();
        Thread *thread = nullptr;
        thread = &Thread::create(
            [thread, function, callback, promise_ptr](const auto user_data) mutable -> void * {
                UNUSED(user_data);
                try {
                    function();
                    promise_ptr->set_value_at_thread_exit();

                    if (callback != nullptr) {
                        callback();
                    }
                } catch (...) {
                    promise_ptr->set_exception_at_thread_exit(std::current_exception());
                }

                thread->destroy();

                return nullptr;
            },
        nullptr);

        return future;
    }
}

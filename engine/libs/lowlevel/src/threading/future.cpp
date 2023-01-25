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

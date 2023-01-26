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

#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/threading/thread_pool.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <utility>

namespace argus {
    typedef std::function<void *(void)> WorkerFunction;

    struct ThreadPoolTask {
        WorkerFunction func;
        std::promise<void *> promise;

        ThreadPoolTask(void);

        ThreadPoolTask(WorkerFunction func);

        ThreadPoolTask(ThreadPoolTask &&) noexcept;
    };

    class ThreadPoolWorker {
      public:
        std::atomic_bool busy{};

      private:
        ThreadPool &pool;
        ThreadPoolTask *current_task{};
        std::deque<ThreadPoolTask *> task_queue;
        std::recursive_mutex task_queue_mutex;
        std::condition_variable_any cond;
        std::atomic_bool terminate{};
        std::thread thread;

        void worker_impl(void);

      public:
        ThreadPoolWorker(ThreadPool &pool);

        ThreadPoolWorker(ThreadPoolWorker &&) noexcept;

        ~ThreadPoolWorker(void);

        std::future<void *> add_task(const WorkerFunction& func);

        void notify(void);

        void halt(void);
    };
}

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
    typedef std::function<void*(void)> WorkerFunction;

    struct ThreadPoolTask {
        WorkerFunction func;
        std::promise<void*> promise;

        ThreadPoolTask(void);

        ThreadPoolTask(WorkerFunction func);

        ThreadPoolTask(ThreadPoolTask&&);
    };

    class ThreadPoolWorker {
        private:
            ThreadPool &pool;
            std::thread thread;
            std::unique_ptr<ThreadPoolTask> current_task;
            std::condition_variable cond;
            std::unique_lock<std::mutex> cond_lock;
            std::unique_lock<std::mutex> cond_mutex;
            std::atomic_bool terminate;

            void worker_impl(void);

        public:
            std::deque<std::unique_ptr<ThreadPoolTask>> task_queue;
            std::mutex task_queue_mutex;
            std::atomic_bool busy;

            ThreadPoolWorker(ThreadPool &pool);

            std::future<void*> add_task(WorkerFunction func);

            void notify(void);

            void halt(void);
    };
}

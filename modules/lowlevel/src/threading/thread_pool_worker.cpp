/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/time.hpp"
#include "argus/lowlevel/threading/future.hpp"
#include "argus/lowlevel/threading/thread_pool.hpp"
#include "internal/lowlevel/pimpl/threading/thread_pool.hpp"
#include "internal/lowlevel/threading/thread_pool_worker.hpp"

#include <functional>
#include <future>
#include <memory>

namespace argus {
    static AllocPool g_task_pool(sizeof(ThreadPoolTask));

    ThreadPoolTask::ThreadPoolTask(void):
            ThreadPoolTask(nullptr) {
    }

    ThreadPoolTask::ThreadPoolTask(ThreadPoolTask &&rhs):
            func(rhs.func) {
        promise.swap(rhs.promise);
    }

    ThreadPoolTask::ThreadPoolTask(WorkerFunction func):
            func(func) {
    }

    ThreadPoolWorker::ThreadPoolWorker(ThreadPool &pool):
            busy(false),
            pool(pool),
            current_task(nullptr),
            task_queue(),
            task_queue_mutex(),
            cond(),
            terminate(false),
            thread(std::bind(&ThreadPoolWorker::worker_impl, this)) {
    }

    ThreadPoolWorker::ThreadPoolWorker(ThreadPoolWorker &&rhs):
            pool(rhs.pool) {
    }

    ThreadPoolWorker::~ThreadPoolWorker(void) {
        terminate = true;
        notify();
        thread.join();
    }

    std::future<void*> ThreadPoolWorker::add_task(WorkerFunction func) {
        task_queue_mutex.lock();
        auto &task = g_task_pool.construct<ThreadPoolTask>(func);
        
        std::promise<void*> &promise = task.promise;
        // this is required so that the thread assigning tasks gains ownership of the future before execution begins
        std::future<void*> future = promise.get_future();

        task_queue.push_back(&task);

        task_queue_mutex.unlock();

        notify();

        return future;
    }

    void ThreadPoolWorker::notify(void) {
        std::lock_guard<std::recursive_mutex> cond_lock(task_queue_mutex);
        cond.notify_one();
    }

    void ThreadPoolWorker::halt(void) {
        terminate = true;
        thread.join();
    }

    void ThreadPoolWorker::worker_impl() {
        // Spin-lock isn't great but it's not for very long and avoids a little
        // complexity.
        //
        // This is also kind of a hack - currently we don't do any initialization
        // after assigning the pimpl member, so this check is guaranteed to be
        // sufficient as long as we don't start doing more initialization.
        // Ideally we'd use a bool member to track whether the pool is ready,
        // but we would need to put it in the ThreadPool object (and not the
        // pimpl) and I'd rather have this small hack than bleed implementation
        // details into the API.
        while (pool.pimpl == nullptr) {
            continue;
        }

        while (true) {
            if (terminate) {
                return;
            }

            {
                std::unique_lock<std::recursive_mutex> task_queue_lock(task_queue_mutex);

                if (task_queue.size() > 0) {
                    current_task = std::move(task_queue.front());
                    busy = true;
                    task_queue.pop_front();
                } else {
                    // try to steal task from another worker
                    //TODO: this might actually be much slower in the case of a
                    //      thread pool that only has a few tasks at a time
                    for (auto &worker : pool.pimpl->workers) {
                        if (!worker->task_queue_mutex.try_lock()) {
                            continue;
                        }

                        if (worker->task_queue.size() > 0) {
                            current_task = std::move(worker->task_queue.back());
                            worker->task_queue.pop_back();
                            worker->task_queue_mutex.unlock();
                            break;
                        } else {
                            worker->task_queue_mutex.unlock();
                        }
                    }

                    if (current_task == nullptr) {
                        busy = false;

                        // wait until we do have a task to run
                        cond.wait(task_queue_lock);
                        continue;
                    }
                }
            }

            // this is self-explanatory
            try {
                void *rv = current_task->func();
                current_task->promise.set_value(rv);
            } catch (...) {
                current_task->promise.set_exception(std::make_exception_ptr(std::current_exception));
            }

            g_task_pool.destroy(current_task);
            current_task = nullptr;
        }
    }
}

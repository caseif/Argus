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
            pool(pool),
            thread(std::bind(&ThreadPoolWorker::worker_impl, this)) {
    }

    std::future<void*> ThreadPoolWorker::add_task(WorkerFunction func) {
        task_queue_mutex.lock();
        task_queue.push_back(std::unique_ptr<ThreadPoolTask>(new ThreadPoolTask(func)));
        std::promise<void*> &promise = task_queue.back()->promise;
        task_queue_mutex.unlock();

        notify();

        return promise.get_future();
    }
    
    void ThreadPoolWorker::notify(void) {
        cond.notify_one();
    }

    void ThreadPoolWorker::halt(void) {
        terminate = true;
        thread.join();
    }

    void ThreadPoolWorker::worker_impl() {
        std::mutex cond_mutex;
        std::unique_lock<std::mutex> cond_lock(cond_mutex);

        busy = false;

        // wait until we have an iniital task to run
        cond.wait(cond_lock);

        while (true) {
            if (terminate) {
                return;
            }

            task_queue_mutex.lock();
            // check whether we have tasks waiting to be run
            if (task_queue.size() > 0) {
                current_task = std::move(task_queue.front());
                busy = true;
                task_queue.pop_front();

                task_queue_mutex.unlock();
            } else {
                // try to steal work from another worker
                bool stole_task = false;
                for (auto &worker : pool.pimpl->workers) {
                    if (!worker->task_queue_mutex.try_lock()) {
                        continue;
                    }
                    if (worker->task_queue.size() > 0) {
                        current_task = std::move(worker->task_queue.back());
                        worker->task_queue.pop_back();
                        worker->task_queue_mutex.unlock();
                        stole_task = true;
                        break;
                    } else {
                        worker->task_queue_mutex.unlock();
                    }
                }

                if (!stole_task) {
                    busy = false;

                    task_queue_mutex.unlock();
                    // wait until we do have a task to run
                    cond.wait(cond_lock);
                    // skip to the beginning of the loop so that we can check the queue and move the task
                    continue;
                }
            }

            // this is self-explanatory
            try {
                void *rv = current_task->func();
                current_task->promise.set_value(rv);
            } catch (...) {
                current_task->promise.set_exception(std::make_exception_ptr(std::current_exception));
            }
        }
    }
}

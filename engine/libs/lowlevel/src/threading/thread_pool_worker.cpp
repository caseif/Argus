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

#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/threading/thread_pool.hpp"
#include "internal/lowlevel/pimpl/threading/thread_pool.hpp"
#include "internal/lowlevel/threading/thread_pool_worker.hpp"

#include <future>
#include <memory>
#include <utility>

namespace argus {
    static PoolAllocator g_task_pool(sizeof(ThreadPoolTask));

    ThreadPoolTask::ThreadPoolTask(void) :
            ThreadPoolTask(nullptr) {
    }

    ThreadPoolTask::ThreadPoolTask(ThreadPoolTask &&rhs) noexcept:
        func(std::move(rhs.func)) {
        promise.swap(rhs.promise);
    }

    ThreadPoolTask::ThreadPoolTask(WorkerFunction func) :
        func(std::move(func)) {
    }

    ThreadPoolWorker::ThreadPoolWorker(ThreadPool &pool) :
        busy(false),
        m_pool(pool),
        m_current_task(nullptr),
        m_task_queue(),
        m_task_queue_mutex(),
        m_cond(),
        m_terminate(false),
        m_thread([this] { worker_impl(); }) {
    }

    ThreadPoolWorker::ThreadPoolWorker(ThreadPoolWorker &&rhs) noexcept:
            m_pool(rhs.m_pool) {
    }

    ThreadPoolWorker::~ThreadPoolWorker(void) {
        m_terminate = true;
        notify();
        m_thread.join();
    }

    std::future<void *> ThreadPoolWorker::add_task(const WorkerFunction &func) {
        m_task_queue_mutex.lock();
        auto &task = g_task_pool.construct<ThreadPoolTask>(func);

        std::promise<void *> &promise = task.promise;
        // this is required so that the thread assigning tasks gains ownership of the future before execution begins
        std::future<void *> future = promise.get_future();

        m_task_queue.push_back(&task);

        m_task_queue_mutex.unlock();

        notify();

        return future;
    }

    void ThreadPoolWorker::notify(void) {
        std::lock_guard<std::recursive_mutex> cond_lock(m_task_queue_mutex);
        m_cond.notify_one();
    }

    void ThreadPoolWorker::halt(void) {
        m_terminate = true;
        m_thread.join();
    }

    void ThreadPoolWorker::worker_impl() {
        // Spin-lock isn't great but it's not for very long and avoids a little
        // complexity.
        //
        // This is also kind of a hack - currently we don't do any initialization
        // after assigning the m_pimpl member, so this check is guaranteed to be
        // sufficient as long as we don't start doing more initialization.
        // Ideally we'd use a bool member to track whether the pool is ready,
        // but we would need to put it in the ThreadPool object (and not the
        // m_pimpl) and I'd rather have this small hack than bleed implementation
        // details into the API.
        while (m_pool.m_pimpl == nullptr) {
            continue;
        }

        while (true) {
            if (m_terminate) {
                return;
            }

            {
                std::unique_lock<std::recursive_mutex> task_queue_lock(m_task_queue_mutex);

                if (m_task_queue.empty()) {
                    // try to steal task from another worker
                    //TODO: this might actually be much slower in the case of a
                    //      thread pool that only has a few tasks at a time
                    for (auto &worker : m_pool.m_pimpl->workers) {
                        if (!worker->m_task_queue_mutex.try_lock()) {
                            continue;
                        }

                        if (worker->m_task_queue.empty()) {
                            worker->m_task_queue_mutex.unlock();
                        } else {
                            m_current_task = worker->m_task_queue.back();
                            worker->m_task_queue.pop_back();
                            worker->m_task_queue_mutex.unlock();
                            break;
                        }
                    }

                    if (m_current_task == nullptr) {
                        busy = false;

                        // wait until we do have a task to run
                        m_cond.wait(task_queue_lock);
                        continue;
                    }
                } else {
                    m_current_task = m_task_queue.front();
                    busy = true;
                    m_task_queue.pop_front();
                }
            }

            // this is self-explanatory
            try {
                void *rv = m_current_task->func();
                m_current_task->promise.set_value(rv);
            } catch (...) {
                m_current_task->promise.set_exception(std::make_exception_ptr(std::current_exception));
            }

            g_task_pool.destroy(m_current_task);
            m_current_task = nullptr;
        }
    }
}

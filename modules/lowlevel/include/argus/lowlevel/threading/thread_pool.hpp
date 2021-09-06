/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <functional>
#include <future>

#include <cstdint>

namespace argus {
    /**
     * \brief For internal use only.
     *
     * \sa ThreadPool
     */
    struct pimpl_ThreadPool;

    /**
     * \brief A pool of threads to which tasks may be assigned.
     *
     * The pool will attempt to automatically balance the workload across the
     * available threads to ensure efficiency.
     */
    class ThreadPool {
        public:
            pimpl_ThreadPool *pimpl;

            /**
             * \brief Constructs a new ThreadPool with the thread count being
             * initialized automatically based on the number of available
             * logical cores.
             */
            ThreadPool(void);

            /**
             * \brief Constructs a new ThreadPool with a fixed number of
             *        threads.
             *
             * \remark Providing a fixed thread count is generally discouraged
             *         unless you know what you're doing - the nullary
             *         constructor is recommended for most use cases.
             */
            ThreadPool(uint16_t threads);

            ThreadPool(const ThreadPool&) = delete;

            ThreadPool(ThreadPool&&) = delete;

            ~ThreadPool(void);

            /**
             * \brief Returns whether the ThreadPool is full initialized and
             *        ready for use.
             *
             * \return Whether the ThreadPool is ready.
             *
             * \remark This is primarily intended for internal use during
             *         construction of the pool and its workers.
             */
            bool is_ready(void) const;

            /**
             * \brief Submits a new task to the ThreadPool.
             *
             * The pointer returned by the callback will be passed back through
             * the std::future returned by this function.
             *
             * \param task The callback which the ThreadPool will invoke to
             *        complete the task.
             * \return A std::future representing the result of the task
             *         execution.
             */
            std::future<void*> submit(std::function<void*(void)> task);

            /**
             * \brief Submits a new task to the ThreadPool, passing the extra
             *        parameter through to the provided callback.
             *
             * The pointer returned by the callback will be passed back through
             * the std::future returned by this function.
             *
             * \param task The callback which the ThreadPool will invoke to
             *        complete the task.
             * \param param A parameter which will be passed through to the
             *        callback.
             * \return A std::future representing the result of the task
             *         execution.
             */
            std::future<void*> submit(std::function<void*(void*)> task, void *param) {
                return this->submit(std::bind(task, param));
            }

            /**
             * \brief Submits a new task to the ThreadPool.
             *
             * The pointer returned by the callback will be passed back through
             * the std::future returned by this function.
             *
             * \tparam R The type returned by the callback function.
             * \param task The callback which the ThreadPool will invoke to
             *        complete the task.
             * \return A std::future representing the result of the task
             *         execution.
             */
            template <typename R>
            std::future<R*> submit(std::function<R*(void)> task) {
                return static_cast<R*>(this->submit(task));
            }

            /**
             * \brief Submits a new task to the ThreadPool, passing the extra
             *        parameter through to the provided callback.
             *
             * The pointer returned by the callback will be passed back through
             * the std::future returned by this function.
             *
             * \tparam R The type returned by the callback function.
             * \tparam P The type of the parameter accepted by the callback
             *         function.
             * \param task The callback which the ThreadPool will invoke to
             *        complete the task.
             * \param param A parameter which will be passed through to the
             *        callback.
             * \return A std::future representing the result of the task
             *         execution.
             */
            template <typename R, typename P>
            std::future<R*> submit(std::function<R*(P*)> task, P *param) {
                return static_cast<R*>(this->submit(task, static_cast<void*>(param)));
            }
    };
}

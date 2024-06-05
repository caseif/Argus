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

#include <functional>
#include <future>

#include <cstdint>

namespace argus {
    /**
     * @brief For internal use only.
     *
     * @sa ThreadPool
     */
    struct pimpl_ThreadPool;

    /**
     * @brief A pool of threads to which tasks may be assigned.
     *
     * The pool will attempt to automatically balance the workload across the
     * available threads to ensure efficiency.
     */
    class ThreadPool {
      public:
        pimpl_ThreadPool *m_pimpl;

        /**
         * @brief Constructs a new ThreadPool with the thread count being
         * initialized automatically based on the number of available
         * logical cores.
         */
        ThreadPool(void);

        /**
         * @brief Constructs a new ThreadPool with a fixed number of
         *        threads.
         *
         * @remark Providing a fixed thread count is generally discouraged
         *         unless you know what you're doing - the nullary
         *         constructor is recommended for most use cases.
         */
        ThreadPool(uint16_t threads);

        ThreadPool(const ThreadPool &) = delete;

        ThreadPool(ThreadPool &&) = delete;

        ~ThreadPool(void);

        /**
         * @brief Submits a new task to the ThreadPool.
         *
         * The pointer returned by the callback will be passed back through
         * the std::future returned by this function.
         *
         * @param task The callback which the ThreadPool will invoke to
         *        complete the task.
         * @return A std::future representing the result of the task
         *         execution.
         */
        std::future<void *> submit(const std::function<void *(void)> &task);

        /**
         * @brief Submits a new task to the ThreadPool, passing the extra
         *        parameter through to the provided callback.
         *
         * The pointer returned by the callback will be passed back through
         * the std::future returned by this function.
         *
         * @param task The callback which the ThreadPool will invoke to
         *        complete the task.
         * @param param A parameter which will be passed through to the
         *        callback.
         * @return A std::future representing the result of the task
         *         execution.
         */
        std::future<void *> submit(const std::function<void *(void *)> &task, void *param) {
            return this->submit([task, param] { return task(param); });
        }

        /**
         * @brief Submits a new task to the ThreadPool.
         *
         * The pointer returned by the callback will be passed back through
         * the std::future returned by this function.
         *
         * @tparam R The type returned by the callback function.
         * @param task The callback which the ThreadPool will invoke to
         *        complete the task.
         * @return A std::future representing the result of the task
         *         execution.
         */
        template<typename R>
        std::future<R *> submit(std::function<R *(void)> task) {
            return static_cast<R *>(this->submit(task));
        }

        /**
         * @brief Submits a new task to the ThreadPool, passing the extra
         *        parameter through to the provided callback.
         *
         * The pointer returned by the callback will be passed back through
         * the std::future returned by this function.
         *
         * @tparam R The type returned by the callback function.
         * @tparam P The type of the parameter accepted by the callback
         *         function.
         * @param task The callback which the ThreadPool will invoke to
         *        complete the task.
         * @param param A parameter which will be passed through to the
         *        callback.
         * @return A std::future representing the result of the task
         *         execution.
         */
        template<typename R, typename P>
        std::future<R *> submit(std::function<R *(P *)> task, P *param) {
            return static_cast<R *>(this->submit(task, static_cast<void *>(param)));
        }
    };
}

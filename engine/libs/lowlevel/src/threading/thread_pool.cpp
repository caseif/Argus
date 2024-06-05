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

#include <future>
#include <thread>

#include <cstdint>

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_ThreadPool));

    static uint16_t _decide_optimal_thread_count() {
        auto cores = std::thread::hardware_concurrency();

        if (cores == 0) {
            // this is arbitrary but should always be small since we're just guessing
            return 1;
        } else if (cores == 1) {
            // no sense in doing any multithreading
            return 1;
        } else if (cores == 2) {
            //TODO: not sure if we should forgo multithreading here since the engine has 2 threads already
            return 2;
        } else if (cores == 3) {
            // not sure if we'll ever actually encounter this
            return 2;
        } else if (cores > UINT16_MAX) {
            // ¯\_(ツ)_/¯
            return UINT16_MAX;
        } else {
            // leave 2 cores, one for the OS/background processes and one for the other engine thread
            return uint16_t(cores - 2);
        }
    }

    ThreadPool::ThreadPool(void) :
            ThreadPool(_decide_optimal_thread_count()) {
    }

    ThreadPool::ThreadPool(uint16_t threads) :
            m_pimpl(&g_pimpl_pool.construct<pimpl_ThreadPool>(*this, threads)) {
    }

    ThreadPool::~ThreadPool(void) {
        g_pimpl_pool.destroy(m_pimpl);
    }

    std::future<void *> ThreadPool::submit(const std::function<void *(void)> &task) {
        auto &worker = m_pimpl->workers.at(m_pimpl->next_worker);
        if (++m_pimpl->next_worker >= m_pimpl->thread_count) {
            m_pimpl->next_worker = 0;
        }

        return worker->add_task(task);
    }
}

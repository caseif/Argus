/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/threading/thread.hpp"
#include "argus/lowlevel/threading/thread_pool.hpp"
#include "internal/lowlevel/pimpl/threading/thread_pool.hpp"

#include <functional>
#include <future>
#include <thread>

#include <cstdint>

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_ThreadPool));

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
            return cores - 2;
        }
    }

    ThreadPool::ThreadPool(void):
            ThreadPool(_decide_optimal_thread_count()) {
    }

    ThreadPool::ThreadPool(uint16_t threads):
            pimpl(&g_pimpl_pool.construct<pimpl_ThreadPool>(*this, threads)) {
    }

    ThreadPool::~ThreadPool(void) {
        g_pimpl_pool.destroy(pimpl);
    }

    std::future<void*> ThreadPool::submit(std::function<void*(void)> task) {
        auto &worker = pimpl->workers.at(pimpl->next_worker);
        if (++pimpl->next_worker >= pimpl->thread_count) {
            pimpl->next_worker = 0;
        }

        worker->add_task(task);
    }
}

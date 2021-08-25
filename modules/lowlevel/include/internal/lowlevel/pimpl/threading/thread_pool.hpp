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
#include "argus/lowlevel/threading/thread_pool.hpp"
#include "internal/lowlevel/threading/thread_pool_worker.hpp"

#include <list>

#include <cstdint>

namespace argus {
    struct pimpl_ThreadPool {
        uint16_t thread_count;
        std::list<ThreadPoolWorker> workers;

        pimpl_ThreadPool(ThreadPool &pool, uint16_t thread_count):
                thread_count(thread_count) {
            for (uint16_t i = 0; i < thread_count; i++) {
                workers.emplace_back(pool);
            }
        }
    };
}

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

#include <memory>
#include <vector>

#include <cstdint>

namespace argus {
    struct pimpl_ThreadPool {
        uint16_t thread_count;
        std::vector<std::unique_ptr<ThreadPoolWorker>> workers;
        uint16_t next_worker;

        pimpl_ThreadPool(ThreadPool &pool, uint16_t thread_count):
                thread_count(thread_count),
                next_worker(0) {
            for (uint16_t i = 0; i < thread_count; i++) {
                workers.emplace_back(new ThreadPoolWorker(pool));
            }
        }
    };
}

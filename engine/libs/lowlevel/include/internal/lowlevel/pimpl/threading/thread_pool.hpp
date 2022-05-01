/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/core/event.hpp"

#include <atomic>

#include <cstdint>

namespace argus {
    template <typename T>
    class RefCountable {
        public:

        std::atomic_uint32_t refcount{0};
        T *ptr;

        explicit RefCountable(T *ptr) {
            this->ptr = ptr;
        }

        void acquire(uint32_t count = 1) {
            refcount.fetch_add(count);
        }

        uint32_t release(uint32_t count = 1) {
            return refcount.fetch_sub(count) - count;
        }
    };

    void process_event_queue(TargetThread target_thread);

    void flush_event_listener_queues(TargetThread target_thread);
}

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

namespace argus {
    template <typename T>
    class RefCountable {
        public:

        std::atomic_uint32_t refcount{0};
        T *ptr;

        explicit RefCountable(T *ptr) {
            this->ptr = ptr;
        }

        void acquire() {
            refcount.fetch_add(1);
        }

        uint32_t release() {
            return refcount.fetch_sub(1) - 1;
        }
    };

    void process_event_queue(TargetThread target_thread);

    void flush_event_listener_queues(TargetThread target_thread);
}

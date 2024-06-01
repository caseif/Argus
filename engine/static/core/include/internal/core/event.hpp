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

#include "argus/core/event.hpp"

#include <atomic>

#include <cstdint>

namespace argus {
    template<typename T>
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

    // WARNING: This method is not thread-safe and assumes that we have
    // exclusive access to the event handler callback lists. If you attempt to
    // invoke this while other threads might be reading the lists, you will have
    // a bad time. This should only ever be used after the engine has spun down.
    void deinit_event_handlers(void);
}

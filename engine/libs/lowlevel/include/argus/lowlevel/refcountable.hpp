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

#include "argus/lowlevel/crash.hpp"
#include "argus/lowlevel/debug.hpp"

#include <atomic>
#include <utility>

#include <cstddef>

namespace argus {
    template<typename T, bool Atomic = false>
    struct RefCountable {
        T value;
        std::conditional_t<Atomic, std::atomic_uint32_t, uint32_t> refcount;

        RefCountable(const T &value) :
            value(value),
            refcount(1) {
        }

        RefCountable(T &&value) :
            value(std::move(value)),
            refcount(1) {
        }

        void acquire(uint32_t acquire_count = 1) {
            if constexpr (Atomic) {
                refcount.fetch_add(acquire_count);
            } else {
                refcount += acquire_count;
            }
        }

        size_t release(uint32_t release_count = 1) {
            if constexpr (Atomic) {
                auto prev_val = refcount.fetch_sub(release_count);
                argus_assert_ll(release_count <= prev_val);
                return prev_val - release_count;
            } else {
                argus_assert_ll(release_count <= refcount);
                refcount -= release_count;
                return refcount;
            }
        }

        inline operator T(void) {
            return value;
        }

        T *operator->(void) {
            return &value;
        }
    };

    template<typename T>
    using AtomicRefCountable = RefCountable<T, true>;
}

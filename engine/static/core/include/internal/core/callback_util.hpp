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

#include "argus/lowlevel/atomic.hpp"
#include "internal/lowlevel/logging.hpp"

#include "argus/core/callback.hpp"

#include <algorithm>
#include <functional>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <vector>

namespace argus {
    extern Index g_next_index;
    extern std::mutex g_next_index_mutex;

    /**
     * \brief A callback accepts no parameters.
     */
    typedef std::function<void()> NullaryCallback;

    template <typename ValueType>
    struct IndexedValue {
        Index id;
        ValueType value;

        //NOLINTNEXTLINE(google-explicit-constructor)
        operator ValueType &() const {
            return value;
        }
    };

    // This struct defines the list alongside two mutation queues and a shared
    // mutex. In this way, it facilitates a thread-safe callback list wherein
    // the callbacks themselves may modify the list, i.e. while the list is
    // being iterated.
    template <typename T>
    struct CallbackList {
        std::vector<IndexedValue<T>> list;
        std::queue<IndexedValue<T>> addition_queue;
        std::queue<Index> removal_queue;
        std::shared_mutex list_mutex;
        std::shared_mutex queue_mutex;

        CallbackList(void) {
        }

        CallbackList(const CallbackList &rhs):
                list(rhs.list),
                addition_queue(rhs.addition_queue),
                removal_queue(rhs.removal_queue) {
        }
    };

    // since IndexedValue is templated we have to define the utility functions here

    template <typename T>
    bool remove_from_indexed_vector(std::vector<IndexedValue<T>> &vector, const Index id) {
        auto it = std::find_if(vector.begin(), vector.end(),
                [id](auto callback) { return callback.id == id; });
        if (it != vector.end()) {
            vector.erase(it);
            return true;
        }
        return false;
    }

    template <typename T>
    void flush_callback_list_queues(CallbackList<T> &list) {
        list.queue_mutex.lock_shared();

        // avoid acquiring an exclusive lock unless we actually need to update the list
        if (!list.removal_queue.empty()) {
            list.queue_mutex.unlock_shared(); // VC++ doesn't allow upgrading lock ownership
            // it's important that we lock list_mutex first, since the callback loop has a perpetual lock on it
            // and individual callbacks may invoke _unregister_callback (thus locking queue_mutex).
            // failure to follow this order will cause deadlock.
            list.list_mutex.lock(); // we need to get a lock on the list since we're updating it
            list.queue_mutex.lock();
            while (!list.removal_queue.empty()) {
                Index id = list.removal_queue.front();
                list.removal_queue.pop();
                if (!remove_from_indexed_vector(list.list, id)) {
                    _ARGUS_WARN("Game attempted to unregister unknown callback %zu", id);
                }
            }
            list.queue_mutex.unlock();
            list.list_mutex.unlock();
        } else {
            list.queue_mutex.unlock_shared();
        }

        // same here
        list.queue_mutex.lock_shared();
        if (!list.addition_queue.empty()) {
            list.queue_mutex.unlock_shared();
            // same deal with the ordering
            list.list_mutex.lock();
            list.queue_mutex.lock();
            while (!list.addition_queue.empty()) {
                list.list.insert(list.list.cend(), list.addition_queue.front());
                list.addition_queue.pop();
            }
            list.queue_mutex.unlock();
            list.list_mutex.unlock();
        } else {
            list.queue_mutex.unlock_shared();
        }
    }

    template <typename T>
    Index add_callback(CallbackList<T> &list, T callback) {
        g_next_index_mutex.lock();
        Index index = g_next_index++;
        g_next_index_mutex.unlock();

        list.queue_mutex.lock();
        list.addition_queue.push({index, callback});
        list.queue_mutex.unlock();

        return index;
    }

    template <typename T>
    void remove_callback(CallbackList<T> &list, const Index index) {
        list.queue_mutex.lock();
        list.removal_queue.push(index);
        list.queue_mutex.unlock();
    }

    template <typename T>
    bool try_remove_callback(CallbackList<T> &list, const Index index) {
        list.list_mutex.lock_shared();
        auto it = std::find_if(list.list.cbegin(), list.list.cend(),
            [index](auto callback) { return callback.id == index; });
        bool present = it != list.list.cend();
        list.list_mutex.unlock_shared();

        if (present) {
            remove_callback(list, index);
        }

        return present;
    }
}

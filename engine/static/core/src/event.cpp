/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/atomic.hpp"
#include "internal/lowlevel/logging.hpp"

#include "argus/core/callback.hpp"
#include "argus/core/event.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/event.hpp"
#include "internal/core/module_core.hpp"

#include <algorithm>
#include <functional>
#include <mutex>
#include <queue>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <vector>

namespace argus {
    struct ArgusEventHandler {
        std::type_index type;
        ArgusEventCallback callback;
        void *data;
    };

    static CallbackList<ArgusEventHandler> g_update_event_listeners;
    static CallbackList<ArgusEventHandler> g_render_event_listeners;

    static std::queue<RefCountable<ArgusEvent>*> g_update_event_queue;
    static std::mutex g_update_event_queue_mutex;
    
    static std::queue<RefCountable<ArgusEvent>*> g_render_event_queue;
    static std::mutex g_render_event_queue_mutex;

    void process_event_queue(const TargetThread target_thread) {
        _ARGUS_ASSERT(target_thread == TargetThread::Update || target_thread == TargetThread::Render,
            "Unrecognized target thread ordinal %u", static_cast<unsigned int>(target_thread));

        auto render_thread = target_thread == TargetThread::Render;

        auto &queue = render_thread ? g_render_event_queue : g_update_event_queue;
        auto &mutex = render_thread ? g_render_event_queue_mutex : g_update_event_queue_mutex;
        auto &listeners = render_thread ? g_render_event_listeners : g_update_event_listeners;

        // We copy the queue so that we're not holding onto the mutex while we
        // execute listener callbacks. Otherwise, dispatching an event from a
        // listener would result in deadlock since it wouldn't be able to
        // re-lock the queue.
        mutex.lock();
        auto queue_copy = queue;
        std::queue<RefCountable<ArgusEvent>*>().swap(queue); // clear queue
        mutex.unlock();

        listeners.list_mutex.lock_shared();

        while (!queue_copy.empty()) {
            auto &event = *queue_copy.front();
            auto listeners_copy = listeners;
            for (const IndexedValue<ArgusEventHandler> &listener : listeners.list) {
                if (static_cast<int>(listener.value.type == event.ptr->type)) {
                    listener.value.callback(*event.ptr, listener.value.data);
                }
            }

            auto rc = event.release();
            if (rc == 0) {
                delete event.ptr;
                delete &event;
            }

            queue_copy.pop();
        }

        listeners.list_mutex.unlock_shared();
    }

    void flush_event_listener_queues(const TargetThread target_thread) {
        CallbackList<ArgusEventHandler> *listeners = nullptr;
        switch (target_thread) {
            case TargetThread::Update: {
                listeners = &g_update_event_listeners;
                break;
            }
            case TargetThread::Render: {
                listeners = &g_render_event_listeners;
                break;
            }
            default: {
                _ARGUS_FATAL("Unrecognized target thread ordinal %u", static_cast<unsigned int>(target_thread));
            }
        }

        flush_callback_list_queues(*listeners);
    }

    Index register_event_handler_with_type(std::type_index type, const ArgusEventCallback &callback,
            const TargetThread target_thread, void *const data) {
        _ARGUS_ASSERT(g_core_initializing || g_core_initialized, "Cannot register event listener before engine initialization.");
        _ARGUS_ASSERT(callback != nullptr, "Event listener cannot have null callback.");

        CallbackList<ArgusEventHandler> *listeners = nullptr;
        switch (target_thread) {
            case TargetThread::Update: {
                listeners = &g_update_event_listeners;
                break;
            }
            case TargetThread::Render: {
                listeners = &g_render_event_listeners;
                break;
            }
            default: {
                _ARGUS_FATAL("Unrecognized target thread ordinal %u", static_cast<unsigned int>(target_thread));
            }
        }

        ArgusEventHandler listener = {type, callback, data};
        return add_callback(*listeners, listener);
    }

    void unregister_event_handler(const Index id) {
        if (!try_remove_callback(g_update_event_listeners, id)) {
            remove_callback(g_render_event_listeners, id);
        }
    }

    void _dispatch_event_ptr(ArgusEvent &event) {
        // we push it to multiple queues so that each thread can pop its queue
        // without affecting the other

        auto event_ref = new RefCountable<ArgusEvent>(&event);

        event_ref->acquire(2);

        g_update_event_queue_mutex.lock();
        g_update_event_queue.push(event_ref);
        g_update_event_queue_mutex.unlock();

        g_render_event_queue_mutex.lock();
        g_render_event_queue.push(event_ref);
        g_render_event_queue_mutex.unlock();
    }

    ArgusEvent::ArgusEvent(std::type_index type):
        type(type) {
    }
}

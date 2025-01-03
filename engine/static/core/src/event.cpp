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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/refcountable.hpp"

#include "argus/core/event.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/event.hpp"
#include "internal/core/module_core.hpp"

#include <functional>
#include <mutex>
#include <queue>
#include <vector>

namespace argus {
    struct ArgusEventHandler {
        std::string type_id;
        ArgusEventWithDataCallback callback;
        void *data;
        ArgusEventHandlerUnregisterCallback unregister_callback;
    };

    static CallbackList<ArgusEventHandler> g_update_event_listeners;
    static CallbackList<ArgusEventHandler> g_render_event_listeners;

    static std::queue<AtomicRefCountable<ArgusEvent *> *> g_update_event_queue;
    static std::mutex g_update_event_queue_mutex;

    static std::queue<AtomicRefCountable<ArgusEvent *> *> g_render_event_queue;
    static std::mutex g_render_event_queue_mutex;

    void process_event_queue(TargetThread target_thread) {
        argus_assert(target_thread == TargetThread::Update || target_thread == TargetThread::Render);

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
        std::queue<AtomicRefCountable<ArgusEvent *> *>().swap(queue); // clear queue
        mutex.unlock();

        listeners.list_mutex.lock_shared();

        while (!queue_copy.empty()) {
            auto &event = *queue_copy.front();

            auto listeners_copy = listeners.lists;
            for (auto ordering : ORDERINGS) {
                for (auto &listener : listeners_copy[ordering]) {
                    if (int(listener.value.type_id == event.value->type_id)) {
                        listener.value.callback(*event.value, listener.value.data);
                    }
                }
            }

            auto rc = event.release();
            if (rc == 0) {
                delete event.value;
                delete &event;
            }

            queue_copy.pop();
        }

        listeners.list_mutex.unlock_shared();
    }

    void flush_event_listener_queues(TargetThread target_thread) {
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
                crash("Unrecognized target thread ordinal %u", unsigned(target_thread));
            }
        }

        flush_callback_list_queues(*listeners);
    }

    Index register_event_handler_with_type(std::string type_id, ArgusEventWithDataCallback callback,
            const TargetThread target_thread, void *const data, Ordering ordering,
            ArgusEventHandlerUnregisterCallback unregister_callback) {
        affirm_precond(is_current_thread_update_thread(),
                "Event handlers may only be registered from the update thread");

        affirm_precond(g_core_initializing || g_core_initialized,
                "Cannot register event listener before engine initialization.");
        affirm_precond(callback != nullptr, "Event listener cannot have null callback.");

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
                crash("Unrecognized target thread ordinal %u", unsigned(target_thread));
            }
        }

        ArgusEventHandler listener = { std::move(type_id), std::move(callback), data, unregister_callback };
        auto index = add_callback(*listeners, listener, ordering);
        return index;
    }

    static void _handle_unregister(Index id, ArgusEventHandler *handler) {
        if (handler->unregister_callback != nullptr) {
            if (is_current_thread_update_thread()) {
                handler->unregister_callback(id, handler->data);
            } else {
                run_on_game_thread([&handler, &id]() { handler->unregister_callback(id, handler->data); });
            }
        }
    }

    void unregister_event_handler(const Index id) {
        if (!try_remove_callback(g_update_event_listeners, id, _handle_unregister)) {
            remove_callback(g_render_event_listeners, id, _handle_unregister);
        }
    }

    void deinit_event_handlers(void) {
        g_update_event_listeners.lists.clear();
        g_render_event_listeners.lists.clear();
    }

    void _dispatch_event_ptr(ArgusEvent &event) {
        // we push it to multiple queues so that each thread can pop its queue
        // without affecting the other

        auto event_ref = new AtomicRefCountable<ArgusEvent *>(&event);

        // RC starts at 1, add 1 more since we're pushing to two queues
        event_ref->acquire(1);

        g_update_event_queue_mutex.lock();
        g_update_event_queue.push(event_ref);
        g_update_event_queue_mutex.unlock();

        g_render_event_queue_mutex.lock();
        g_render_event_queue.push(event_ref);
        g_render_event_queue_mutex.unlock();
    }

    ArgusEvent::ArgusEvent(std::string type_id) :
            type_id(std::move(type_id)) {
    }

    ArgusEvent::ArgusEvent(const ArgusEvent &rhs) = default;
}

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/threading.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/callback.hpp"
#include "argus/core/event.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/event.hpp"
#include "internal/core/module_core.hpp"

#include <algorithm>
#include <functional>
#include <mutex>
#include <queue>
#include <type_traits>
#include <vector>

#include <cstdlib>
#include <cstring>

namespace argus {
    struct ArgusEventHandler {
        ArgusEventType type;
        ArgusEventCallback callback;
        void *data;
    };

    static CallbackList<ArgusEventHandler> g_update_event_listeners;
    static CallbackList<ArgusEventHandler> g_render_event_listeners;

    static std::queue<RefCountable<ArgusEvent>*> g_update_event_queue;
    static std::mutex g_update_event_queue_mutex;
    
    static std::queue<RefCountable<ArgusEvent>*> g_render_event_queue;
    static std::mutex g_render_event_queue_mutex;

    constexpr inline ArgusEventType operator|(const ArgusEventType lhs, const ArgusEventType rhs) {
        return static_cast<ArgusEventType>(static_cast<std::underlying_type<ArgusEventType>::type>(lhs) |
                                           static_cast<std::underlying_type<ArgusEventType>::type>(rhs));
    }

    inline ArgusEventType operator|=(ArgusEventType &lhs, const ArgusEventType rhs) {
        return lhs = static_cast<ArgusEventType>(static_cast<std::underlying_type<ArgusEventType>::type>(lhs) |
                                           static_cast<std::underlying_type<ArgusEventType>::type>(rhs));
    }

    constexpr inline ArgusEventType operator&(const ArgusEventType lhs, const ArgusEventType rhs) {
        return static_cast<ArgusEventType>(static_cast<std::underlying_type<ArgusEventType>::type>(lhs) &
                                           static_cast<std::underlying_type<ArgusEventType>::type>(rhs));
    }

    void process_event_queue(const TargetThread target_thread) {
        _ARGUS_ASSERT(target_thread == TargetThread::UPDATE || target_thread == TargetThread::RENDER,
            "Unrecognized target thread ordinal %u\n", (unsigned int) target_thread);

        auto render_thread = target_thread == TargetThread::RENDER;

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
            for (IndexedValue<ArgusEventHandler> listener : listeners.list) {
                if (static_cast<int>(listener.value.type & event.ptr->type)) {
                    listener.value.callback(*event.ptr, listener.value.data);
                }
            }

            auto rc = event.release();
            if (rc == 0) {
                free(event.ptr);
                delete &event;
            }

            queue_copy.pop();
        }

        listeners.list_mutex.unlock_shared();
    }

    void flush_event_listener_queues(const TargetThread target_thread) {
        CallbackList<ArgusEventHandler> *listeners;
        switch (target_thread) {
            case TargetThread::UPDATE: {
                listeners = &g_update_event_listeners;
                break;
            }
            case TargetThread::RENDER: {
                listeners = &g_render_event_listeners;
                break;
            }
            default: {
                _ARGUS_FATAL("Unrecognized target thread ordinal %u\n", (unsigned int) target_thread);
            }
        }

        flush_callback_list_queues(*listeners);
    }

    const Index register_event_handler(const ArgusEventType type, const ArgusEventCallback callback,
            const TargetThread target_thread, void *const data) {
        _ARGUS_ASSERT(g_core_initializing || g_core_initialized, "Cannot register event listener before engine initialization.");
        _ARGUS_ASSERT(callback != nullptr, "Event listener cannot have null callback.");

        CallbackList<ArgusEventHandler> *listeners;
        switch (target_thread) {
            case TargetThread::UPDATE: {
                listeners = &g_update_event_listeners;
                break;
            }
            case TargetThread::RENDER: {
                listeners = &g_render_event_listeners;
                break;
            }
            default: {
                _ARGUS_FATAL("Unrecognized target thread ordinal %u\n", (unsigned int) target_thread);
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

    void _dispatch_event_ptr(const ArgusEvent &event, size_t obj_size) {
        // we push it to multiple queues so that each thread can pop its queue
        // without affecting the other

        // It's difficult to get around these mallocs while also still allowing
        // for event inheritance, since each subclass can have a different size
        // thus preventing us from using an AllocPool or even just directly
        // storing the event data in the queue. Potential solutions include:
        //   - Setting a maximum size. Setting such an arbitrary restriction
        //     seems like a pretty bad hack and lets the implementation guide
        //     the API, which should be avoided at all costs.
        //   - Rolling all the event-specific data into a giant struct,
        //     SDL-style. This means no inheritance, which means modules can't
        //     specify their own events. This is a huge tradeoff in flexibility.
        // In practice, using mallocs (even every frame) doesn't actually seem
        // to incur a noticeable performance hit, so for now it's probably okay
        // to just leave it as a "good enough" solution.
        ArgusEvent *event_copy = static_cast<ArgusEvent*>(malloc(obj_size));
        memcpy(event_copy, &event, obj_size);

        auto event_ref = new RefCountable<ArgusEvent>(event_copy);

        g_update_event_queue_mutex.lock();
        event_ref->acquire();
        g_update_event_queue.push(event_ref);
        g_update_event_queue_mutex.unlock();

        g_render_event_queue_mutex.lock();
        event_ref->acquire();
        g_render_event_queue.push(event_ref);
        g_render_event_queue_mutex.unlock();
    }

    ArgusEvent::ArgusEvent(ArgusEventType type):
        type(type) {
    }
}

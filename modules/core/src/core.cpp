/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/threading.hpp"
#include "argus/time.hpp"
#include "internal/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/config.hpp"
#include "internal/core_util.hpp"
#include "internal/sdl_event.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#include <SDL2/SDL.h>

#define US_PER_S 1000000LLU
#define SLEEP_OVERHEAD_NS 120000LLU

namespace argus {

    EngineModules operator |(const EngineModules lhs, const EngineModules rhs) {
        return static_cast<EngineModules>(
                static_cast<std::underlying_type<EngineModules>::type>(lhs)
                | static_cast<std::underlying_type<EngineModules>::type>(rhs)
        );
    }

    constexpr inline EngineModules operator |=(const EngineModules lhs, const EngineModules rhs) {
        return static_cast<EngineModules>(
                static_cast<std::underlying_type<EngineModules>::type>(lhs)
                | static_cast<std::underlying_type<EngineModules>::type>(rhs)
        );
    }

    inline bool operator &(const EngineModules lhs, const EngineModules rhs) {
        return (static_cast<std::underlying_type<EngineModules>::type>(lhs)
                & static_cast<std::underlying_type<EngineModules>::type>(rhs));
    }

    template <typename Filter, typename Callback>
    struct EventHandler {
        Filter filter;
        Callback callback;
        void *data;
    };

    typedef EventHandler<ArgusEventFilter, ArgusEventCallback> ArgusEventHandler;
    typedef EventHandler<SDLEventFilter, SDLEventCallback> SDLEventHandler;

    // This struct defines the list alongside two mutation queues and a shared
    // mutex. In this way, it facilitates a thread-safe callback list wherein
    // the callbacks themselves may modify the list, i.e. while the list is
    // being iterated.
    template<typename T>
    struct CallbackList {
        std::vector<IndexedValue<T>> list;
        std::queue<IndexedValue<T>> addition_queue;
        std::queue<Index> removal_queue;
        SharedMutex list_mutex;
        SharedMutex queue_mutex;
    };

    Thread *g_render_thread;

    static Index g_next_index = 0;
    static std::mutex g_next_index_mutex;

    static CallbackList<DeltaCallback> g_update_callbacks;
    static CallbackList<DeltaCallback> g_render_callbacks;
    static CallbackList<ArgusEventHandler> g_event_listeners;
    static CallbackList<SDLEventHandler> g_sdl_event_listeners;

    static std::queue<std::unique_ptr<ArgusEvent>> g_event_queue;
    static std::mutex g_event_queue_mutex;

    static EngineModules g_enabled_modules;
    static std::vector<EngineModules> g_all_modules {
            EngineModules::LOWLEVEL,
            EngineModules::CORE,
            EngineModules::RESMAN,
            EngineModules::RENDERER
    };

    static bool g_engine_stopping = false;

    extern EngineConfig g_engine_config;

    bool g_initializing = false;
    bool g_initialized = false;

    // module lifecycle hooks
    extern void update_lifecycle_resman(LifecycleStage);
    extern void update_lifecycle_renderer(LifecycleStage);
    void update_lifecycle_core(LifecycleStage);

    std::map<const EngineModules, const LifecycleUpdateCallback> g_lifecycle_hooks{
        {EngineModules::CORE, update_lifecycle_core},
        {EngineModules::RESMAN, update_lifecycle_resman},
        {EngineModules::RENDERER, update_lifecycle_renderer}
    };

    static void _interrupt_handler(int signal) {
        stop_engine();
    }

    static void _handle_idle(const Timestamp start_timestamp, const unsigned int target_rate) {
        if (target_rate == 0) {
            return;
        }

        TimeDelta delta = argus::microtime() - start_timestamp;

        unsigned int frametime_target_us = US_PER_S / target_rate;
        if (delta < frametime_target_us) {
            unsigned long long sleep_time_ns = (frametime_target_us - delta) * 1000;
            if (sleep_time_ns <= SLEEP_OVERHEAD_NS) {
                return;
            }
            sleep_nanos(sleep_time_ns - SLEEP_OVERHEAD_NS);
        }
    }

    static const TimeDelta _compute_delta(Timestamp &last_timestamp) {
        TimeDelta delta;

        if (last_timestamp != 0) {
            delta = argus::microtime() - last_timestamp;
        } else {
            delta = 0;
        }
        last_timestamp = argus::microtime();

        return delta;
    }

    template<typename T>
    static const bool _remove_from_indexed_vector(std::vector<IndexedValue<T>> &vector, const Index id) {
        auto it = std::remove_if(vector.begin(), vector.end(),
                [id](auto callback) {return callback.id == id;});
        if (it != vector.end()) {
            vector.erase(it, vector.end());
            return true;
        }
        return false;
    }

    template<typename T>
    static void _flush_callback_list_queues(CallbackList<T> &list) {
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
                if (!_remove_from_indexed_vector(list.list, id)) {
                    _ARGUS_WARN("Game attempted to unregister unknown callback %llu\n", id);
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

    static int _master_event_handler(SDL_Event &event) {
        for (IndexedValue<SDLEventHandler> listener : g_sdl_event_listeners.list) {
            if (listener.value.filter == nullptr || listener.value.filter(event, listener.value.data)) {
                listener.value.callback(event, listener.value.data);
            }
        }
        g_sdl_event_listeners.list_mutex.unlock_shared();

        return 0;
    }

    static void _process_event_queue(void) {
        g_event_queue_mutex.lock();
        g_event_listeners.list_mutex.lock_shared();

        while (!g_event_queue.empty()) {
            ArgusEvent &event = *std::move(g_event_queue.front().get());
            for (IndexedValue<ArgusEventHandler> listener : g_event_listeners.list) {
                if (listener.value.filter == nullptr || listener.value.filter(event, listener.value.data)) {
                    listener.value.callback(event, listener.value.data);
                }
            }
            g_event_queue.pop();
        }

        g_event_listeners.list_mutex.unlock_shared();
        g_event_queue_mutex.unlock();
    }

    void _initialize_sdl(void) {
        SDL_Init(0);

        if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0) {
            _ARGUS_FATAL("Failed to initialize SDL events: %s\n", SDL_GetError());
        }
    }

    void update_lifecycle_core(LifecycleStage stage) {
        if (stage == LifecycleStage::PRE_INIT) {
            _ARGUS_ASSERT(!g_initializing && !g_initialized, "Cannot initialize engine more than once.");

            g_initializing = true;

            // we'll probably register around 10 or so internal callbacks, so allocate them now
            g_update_callbacks.list.reserve(10);
            return;
        } else if (stage == LifecycleStage::INIT) {
            _initialize_sdl();

            g_initialized = true;
        } else if (stage == LifecycleStage::POST_DEINIT) {
            g_render_thread->detach();
            g_render_thread->destroy();

            SDL_Quit();
        }
    }

    void _initialize_modules(const EngineModules modules) {
        for (LifecycleStage stage = LifecycleStage::PRE_INIT; stage <= LifecycleStage::POST_INIT;
                stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            for (EngineModules module : g_all_modules) {
                if (modules & module) {
                    auto it = g_lifecycle_hooks.find(module);
                    if (it != g_lifecycle_hooks.cend()) {
                        it->second(stage);
                    }
                }
            }
        }
    }

    void _deinitialize_modules(const EngineModules modules) {
        for (LifecycleStage stage = LifecycleStage::PRE_DEINIT; stage <= LifecycleStage::POST_DEINIT;
                stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            // we want to deinitialize the modules in the opposite order as they were initialized
            if (modules & EngineModules::RENDERER) {
                update_lifecycle_renderer(stage);
            }
            update_lifecycle_core(stage);
        }
    }

    void initialize_engine(const EngineModules modules) {
        signal(SIGINT, _interrupt_handler);
        g_enabled_modules = modules;
        _initialize_modules(modules);
    }

    static void _clean_up(void) {
        _deinitialize_modules(g_enabled_modules);
    }

    static void _game_loop(void) {
        static Timestamp last_update = 0;

        while (1) {
            if (g_engine_stopping) {
                _clean_up();
                break;
            }

            Timestamp update_start = argus::microtime();
            TimeDelta delta = _compute_delta(last_update);

            _flush_callback_list_queues(g_update_callbacks);
            _flush_callback_list_queues(g_render_callbacks);
            _flush_callback_list_queues(g_event_listeners);
            _flush_callback_list_queues(g_sdl_event_listeners);

            // clear event queue
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                _master_event_handler(event);
            }

            _process_event_queue();

            // invoke update callbacks
            g_update_callbacks.list_mutex.lock_shared();
            for (IndexedValue<DeltaCallback> callback : g_update_callbacks.list) {
                callback.value(delta);
            }
            g_update_callbacks.list_mutex.unlock_shared();

            if (g_engine_config.target_tickrate != 0) {
                _handle_idle(update_start, g_engine_config.target_tickrate);
            }
        }

        return;
    }

    static void *_render_loop(void *const _) {
        static Timestamp last_frame = 0;

        while (1) {
            if (g_engine_stopping) {
                break;
            }

            Timestamp render_start = argus::microtime();
            TimeDelta delta = _compute_delta(last_frame);

            // invoke render callbacks
            g_render_callbacks.list_mutex.lock_shared();
            for (IndexedValue<DeltaCallback> callback : g_render_callbacks.list) {
                callback.value(delta);
            }
            g_render_callbacks.list_mutex.unlock_shared();

            if (g_engine_config.target_framerate != 0) {
                _handle_idle(render_start, g_engine_config.target_framerate);
            }
        }

        return nullptr;
    }

    template<typename T>
    Index _add_callback(CallbackList<T> &list, T callback) {
        g_next_index_mutex.lock();
        Index index = g_next_index++;
        g_next_index_mutex.unlock();

        list.queue_mutex.lock();
        list.addition_queue.push({index, callback});
        list.queue_mutex.unlock();

        return index;
    }

    template<typename T>
    void _remove_callback(CallbackList<T> &list, const Index index) {
        list.queue_mutex.lock();
        list.removal_queue.push(index);
        list.queue_mutex.unlock();
    }

    const Index register_update_callback(const DeltaCallback callback) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register update callback before engine initialization.");
        return _add_callback(g_update_callbacks, callback);
    }

    void unregister_update_callback(const Index id) {
        _remove_callback(g_update_callbacks, id);
    }

    const Index register_render_callback(const DeltaCallback callback) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register render callback before engine initialization.");
        return _add_callback(g_render_callbacks, callback);
    }

    void unregister_render_callback(const Index id) {
        _remove_callback(g_render_callbacks, id);
    }

    const Index register_event_handler(const ArgusEventFilter filter, const ArgusEventCallback callback, void *const data) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register event listener before engine initialization.");
        Index id = g_next_index++;
        _ARGUS_ASSERT(callback != nullptr, "Event listener cannot have null callback.");

        ArgusEventHandler listener = {filter, callback, data};
        return _add_callback(g_event_listeners, listener);
    }

    void unregister_event_handler(const Index id) {
        _remove_callback(g_event_listeners, id);
    }

    const Index register_sdl_event_handler(const SDLEventFilter filter, const SDLEventCallback callback, void *const data) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register SDL event listener before engine initialization.");
        Index id = g_next_index++;
        _ARGUS_ASSERT(callback != nullptr, "SDL event listener cannot have null callback.");

        SDLEventHandler listener = {filter, callback, data};

        return _add_callback(g_sdl_event_listeners, listener);
    }

    void unregister_sdl_event_handler(const Index id) {
        _remove_callback(g_sdl_event_listeners, id);
    }

    void _dispatch_event_ptr(std::unique_ptr<ArgusEvent> &&event) {
        g_event_queue.push(std::move(event));
    }

    void start_engine(const DeltaCallback game_loop) {
        _ARGUS_ASSERT(g_initialized, "Cannot start engine before it is initialized.");
        _ARGUS_ASSERT(game_loop != NULL, "start_engine invoked with null callback");

        register_update_callback(game_loop);

        g_render_thread = &Thread::create(_render_loop, nullptr);

        // pass control over to the game loop
        _game_loop();

        exit(0);
    }

    void stop_engine(void) {
        _ARGUS_ASSERT(g_initialized, "Cannot stop engine before it is initialized.");

        g_engine_stopping = true;
    }

}

// module lowlevel
#include "argus/threading.hpp"
#include "argus/time.hpp"
#include "internal/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/config.hpp"
#include "internal/core_util.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <iostream>
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

    typedef struct {
        ArgusEventFilter filter;
        ArgusEventCallback callback;
        void *data;
    } EventHandler;

    // This struct defines the list alongside two mutation queues and a shared
    // mutex. In this way, it facilitates a thread-safe callback list wherein
    // the callbacks themselves may modify the list, i.e. while the list is
    // being iterated.
    template<typename T>
    struct CallbackList {
        std::vector<IndexedValue<T>> list;
        std::queue<IndexedValue<T>> addition_queue;
        std::queue<Index> removal_queue;
        smutex list_mutex;
        smutex queue_mutex;
    };

    Thread *g_render_thread;

    static Index g_next_index = 0;
    static std::mutex g_next_index_mutex;

    static CallbackList<DeltaCallback> g_update_callbacks;
    static CallbackList<DeltaCallback> g_render_callbacks;
    static CallbackList<NullaryCallback> g_close_callbacks;
    static CallbackList<EventHandler> g_event_listeners;

    static bool g_engine_stopping = false;

    extern EngineConfig g_engine_config;

    bool g_initializing = false;
    bool g_initialized = false;

    // module initializers
    extern void init_module_renderer(void);

    static void _interrupt_handler(int signal) {
        stop_engine();
    }

    static void _clean_up(void) {
        // we want to deinitialize the modules in the opposite order as they were initialized
        smutex_lock_shared(g_close_callbacks.list_mutex);
        for (std::vector<IndexedValue<NullaryCallback>>::reverse_iterator it = g_close_callbacks.list.rbegin();
                it != g_close_callbacks.list.rend(); it++) { 
            it->value();
        }
        smutex_unlock_shared(g_close_callbacks.list_mutex);

        g_render_thread->detach();
        g_render_thread->destroy();
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
        smutex_lock_shared(list.queue_mutex);

        // avoid acquiring an exclusive lock unless we actually need to update the list
        if (!list.removal_queue.empty()) {
            smutex_unlock_shared(list.queue_mutex); // VC++ doesn't allow upgrading lock ownership
            // it's important that we lock list_mutex first, since the callback loop has a perpetual lock on it
            // and individual callbacks may invoke _unregister_callback (thus locking queue_mutex).
            // failure to follow this order will cause deadlock.
            smutex_lock(list.list_mutex); // we need to get a lock on the list since we're updating it
            smutex_lock(list.queue_mutex);
            while (!list.removal_queue.empty()) {
                Index id = list.removal_queue.front();
                list.removal_queue.pop();
                if (!_remove_from_indexed_vector(list.list, id)) {
                    _ARGUS_WARN("Game attempted to unregister unknown callback %llu\n", id);
                }
            }
            smutex_unlock(list.queue_mutex);
            smutex_unlock(list.list_mutex);
        } else {
            smutex_unlock_shared(list.queue_mutex);
        }

        // same here
        smutex_lock_shared(list.queue_mutex);
        if (!list.addition_queue.empty()) {
            smutex_unlock_shared(list.queue_mutex);
            // same deal with the ordering
            smutex_lock(list.list_mutex);
            smutex_lock(list.queue_mutex);
            while (!list.addition_queue.empty()) {
                list.list.insert(list.list.cend(), list.addition_queue.front());
                list.addition_queue.pop();
            }
            smutex_unlock(list.queue_mutex);
            smutex_unlock(list.list_mutex);
        } else {
            smutex_unlock_shared(list.queue_mutex);
        }
    }

    template<typename T>
    void _init_callback_list(CallbackList<T> &list) {
        smutex_create(list.list_mutex);
        smutex_create(list.queue_mutex);
    }

    static int _master_event_handler(SDL_Event &event) {
        ArgusEvent argus_event = {UNDEFINED, &event};
        smutex_lock_shared(g_event_listeners.list_mutex);
        for (IndexedValue<EventHandler> listener : g_event_listeners.list) {
            if (listener.value.filter == nullptr || listener.value.filter(argus_event, listener.value.data)) {
                listener.value.callback(argus_event, listener.value.data);
            }
        }
        smutex_unlock_shared(g_event_listeners.list_mutex);

        return 0;
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
            _flush_callback_list_queues(g_close_callbacks);
            _flush_callback_list_queues(g_event_listeners);

            // clear event queue
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                _master_event_handler(event);
            }

            // invoke update callbacks
            smutex_lock_shared(g_update_callbacks.list_mutex);
            for (IndexedValue<DeltaCallback> callback : g_update_callbacks.list) {
                callback.value(delta);
            }
            smutex_unlock_shared(g_update_callbacks.list_mutex);

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
            smutex_lock_shared(g_render_callbacks.list_mutex);
            for (IndexedValue<DeltaCallback> callback : g_render_callbacks.list) {
                callback.value(delta);
            }
            smutex_unlock_shared(g_render_callbacks.list_mutex);

            if (g_engine_config.target_framerate != 0) {
                _handle_idle(render_start, g_engine_config.target_framerate);
            }
        }

        return nullptr;
    }

    void _initialize_sdl(void) {
        SDL_Init(0);

        if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0) {
            _ARGUS_FATAL("Failed to initialize SDL events: %s\n", SDL_GetError());
        }
    }
    
    void _initialize_modules(const EngineModules module_bitmask) {
        if (module_bitmask & EngineModules::RENDERER) {
            init_module_renderer();
        }
    }

    void initialize_engine(const EngineModules module_bitmask) {
        _ARGUS_ASSERT(!g_initializing && !g_initialized, "Cannot initialize engine more than once.");

        g_initializing = true;

        signal(SIGINT, _interrupt_handler);

        // we'll probably register around 10 or so internal callbacks, so allocate them now
        g_update_callbacks.list.reserve(10);

        _init_callback_list(g_update_callbacks);
        _init_callback_list(g_render_callbacks);
        _init_callback_list(g_close_callbacks);
        _init_callback_list(g_event_listeners);

        _initialize_sdl();

        _initialize_modules(module_bitmask);

        g_initialized = true;
        return;
    }

    template<typename T>
    Index _add_callback(CallbackList<T> &list, T callback) {
        g_next_index_mutex.lock();
        Index index = g_next_index++;
        g_next_index_mutex.unlock();

        smutex_lock(list.queue_mutex);
        list.addition_queue.push({index, callback});
        smutex_unlock(list.queue_mutex);

        return index;
    }

    template<typename T>
    void _remove_callback(CallbackList<T> &list, const Index index) {
        smutex_lock(list.queue_mutex);
        list.removal_queue.push(index);
        smutex_unlock(list.queue_mutex);
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

    const Index register_close_callback(const NullaryCallback callback) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register close callback before engine initialization.");
        return _add_callback(g_close_callbacks, callback);
    }

    void unregister_close_callback(const Index id) {
        _remove_callback(g_close_callbacks, id);
    }

    const Index register_event_handler(const ArgusEventFilter filter, const ArgusEventCallback callback, void *const data) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register event listener before engine initialization.");
        Index id = g_next_index++;
        _ARGUS_ASSERT(callback != nullptr, "Event listener cannot have null callback.");

        EventHandler listener = {filter, callback, data};
        return _add_callback(g_event_listeners, listener);
    }

    void unregister_event_handler(const Index id) {
        _remove_callback(g_event_listeners, id);
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

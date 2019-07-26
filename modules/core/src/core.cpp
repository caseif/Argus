// module pi
#include "argus/threading.hpp"
#include "argus/time.hpp"

// module core
#include "argus/core.hpp"
#include "internal/config.hpp"
#include "internal/util.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <vector>

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

    template <typename ValueType>
    struct IndexedValue {
        uint64_t id;
        ValueType value;
    };

    typedef struct {
        SDL_EventFilter filter;
        SDLEventCallback callback;
        void *data;
    } SDLEventListener;

    Thread *g_render_thread;

    static uint64_t g_next_index = 0;

    static std::vector<IndexedValue<DeltaCallback>> g_update_callbacks;
    static std::vector<IndexedValue<DeltaCallback>> g_render_callbacks;
    static std::vector<IndexedValue<NullaryCallback>> g_close_callbacks;

    static std::vector<IndexedValue<SDLEventListener>> g_event_listeners;

    static bool g_engine_stopping = false;

    extern EngineConfig g_engine_config;

    bool g_initializing = false;
    bool g_initialized = false;

    unsigned long long g_last_update = 0;
    unsigned long long g_last_frame = 0;

    // module initializers
    extern void init_module_renderer(void);

    static void _interrupt_handler(int signal) {
        stop_engine();
    }

    static void _clean_up(void) {
        // we want to deinitialize the modules in the opposite order as they were initialized
        for (std::vector<IndexedValue<NullaryCallback>>::reverse_iterator it = g_close_callbacks.rbegin();
                it != g_close_callbacks.rend(); it++) { 
            it->value();
        }

        thread_detach(g_render_thread);
        thread_destroy(g_render_thread);
    }

    static void _handle_idle(unsigned long long start_timestamp, unsigned int target_rate) {
        if (target_rate == 0) {
            return;
        }

        unsigned long long delta = argus::microtime() - start_timestamp;

        unsigned int frametime_target_us = US_PER_S / target_rate;
        if (delta < frametime_target_us) {
            unsigned long long sleep_time_ns = (frametime_target_us - delta) * 1000;
            if (sleep_time_ns <= SLEEP_OVERHEAD_NS) {
                return;
            }
            sleep_nanos(sleep_time_ns - SLEEP_OVERHEAD_NS);
        }
    }

    static unsigned long long _compute_delta(unsigned long long *last_timestamp) {
        unsigned long long delta;

        if (*last_timestamp != 0) {
            delta = argus::microtime() - *last_timestamp;
        } else {
            delta = 0;
        }
        *last_timestamp = argus::microtime();

        return delta;
    }

    static void _game_loop(void) {
        while (1) {
            if (g_engine_stopping) {
                _clean_up();
                break;
            }

            unsigned long long update_start = argus::microtime();
            unsigned long long delta = _compute_delta(&g_last_update);

            // pump events
            SDL_PumpEvents();

            // invoke update callbacks
            std::vector<IndexedValue<DeltaCallback>>::const_iterator it;
            for (it = g_update_callbacks.begin(); it != g_update_callbacks.end(); it++) {
                it->value(delta);
            }

            if (g_engine_config.target_tickrate != 0) {
                _handle_idle(update_start, g_engine_config.target_tickrate);
            }
        }

        return;
    }

    static void *_render_loop(void*) {
        while (1) {
            if (g_engine_stopping) {
                break;
            }

            unsigned long long render_start = argus::microtime();
            unsigned long long delta = _compute_delta(&g_last_frame);

            // invoke render callbacks
            std::vector<IndexedValue<DeltaCallback>>::const_iterator it;
            for (it = g_render_callbacks.begin(); it != g_render_callbacks.end(); it++) {
                it->value(delta);
            }

            if (g_engine_config.target_framerate != 0) {
                _handle_idle(render_start, g_engine_config.target_framerate);
            }
        }

        return nullptr;
    }

    static int _master_event_callback(void *data, SDL_Event *event) {
        std::vector<IndexedValue<SDLEventListener>>::const_iterator it;
        for (IndexedValue<SDLEventListener> listener : g_event_listeners) {
            if (listener.value.filter == nullptr || listener.value.filter(listener.value.data, event)) {
                listener.value.callback(listener.value.data, event);
            }
        }

        return 0;
    }
    
    void _initialize_modules(EngineModules module_bitmask) {
        if (module_bitmask & EngineModules::RENDERER) {
            init_module_renderer();
        }
    }

    void initialize_engine(EngineModules module_bitmask) {
        ASSERT(!g_initializing && !g_initialized, "Cannot initialize engine more than once.");

        g_initializing = true;

        signal(SIGINT, _interrupt_handler);

        // we'll probably register around 10 or so internal callbacks, so allocate them now
        g_update_callbacks.reserve(10);

        _initialize_modules(module_bitmask);

        SDL_AddEventWatch(_master_event_callback, nullptr);

        g_initialized = true;
        return;
    }

    template <typename ValueType>
    static bool remove_from_vector(std::vector<IndexedValue<ValueType>> *vector, uint64_t id) {
        auto it = std::remove_if(vector->begin(), vector->end(),
                [id](auto callback) {return callback.id == id;});
        if (it != vector->end()) {
            vector->erase(it, vector->end());
            return true;
        }
        return false;
    }

    uint64_t register_update_callback(DeltaCallback callback) {
        ASSERT(g_initializing || g_initialized, "Cannot register update callback before engine initialization.");
        uint64_t id = g_next_index++;
        g_update_callbacks.insert(g_update_callbacks.cend(), {id, callback});
        return id;
    }

    bool unregister_update_callback(uint64_t id) {
        if (!remove_from_vector(&g_update_callbacks, id)) {
            WARN("Game attempted to unregister unknown update callback %lu\n", id);
            return false;
        }
        return true;
    }

    uint64_t register_render_callback(DeltaCallback callback) {
        ASSERT(g_initializing || g_initialized, "Cannot register render callback before engine initialization.");
        uint64_t id = g_next_index++;
        g_render_callbacks.insert(g_render_callbacks.cend(), {id, callback});
        return id;
    }

    bool unregister_render_callback(uint64_t id) {
        if (!remove_from_vector(&g_render_callbacks, id)) {
            WARN("Game attempted to unregister unknown render callback %lu\n", id);
            return false;
        }
        return true;
    }

    uint64_t register_close_callback(NullaryCallback callback) {
        ASSERT(g_initializing || g_initialized, "Cannot register close callback before engine initialization.");
        uint64_t id = g_next_index++;
        g_close_callbacks.insert(g_close_callbacks.cend(), {id, callback});
        return id;
    }

    bool unregister_close_callback(uint64_t id) {
        if (!remove_from_vector(&g_close_callbacks, id)) {
            WARN("Game attempted to unregister unknown close callback %lu\n", id);
            return false;
        }
        return true;
    }

    uint64_t register_sdl_event_listener(SDL_EventFilter filter, SDLEventCallback callback, void *data) {
        ASSERT(g_initializing || g_initialized, "Cannot register event listener before engine initialization.");
        uint64_t id = g_next_index++;
        ASSERT(callback != nullptr, "Event listener cannot have null callback.");

        SDLEventListener listener = {filter, callback, data};
        g_event_listeners.insert(g_event_listeners.cend(), {id, listener});
        return id;
    }

    bool unregister_sdl_event_listener(uint64_t id) {
        if (!remove_from_vector(&g_event_listeners, id)) {
            WARN("Game attempted to unregister unknown update callback %lu\n", id);
            return false;
        }
        return true;
    }

    void start_engine(DeltaCallback game_loop) {
        ASSERT(g_initialized, "Cannot start engine before it is initialized.");
        ASSERT(game_loop != NULL, "start_engine invoked with null callback");

        register_update_callback(game_loop);

        g_render_thread = thread_create(_render_loop, nullptr);

        // pass control over to the game loop
        _game_loop();

        exit(0);
    }

    void stop_engine(void) {
        ASSERT(g_initialized, "Cannot stop engine before it is initialized.");

        g_engine_stopping = true;
    }

}

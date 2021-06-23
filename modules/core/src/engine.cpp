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
#include "argus/lowlevel/time.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/callback.hpp"
#include "argus/core/engine.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/event.hpp"
#include "internal/core/module.hpp"
#include "internal/core/module_core.hpp"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <csignal>
#include <cstdint>
#include <cstdlib>

namespace argus {
    /**
     * \brief Represents an instant in time.
     */
    typedef unsigned long long Timestamp;

    static CallbackList<DeltaCallback> g_update_callbacks;
    static CallbackList<DeltaCallback> g_render_callbacks;

    std::map<std::string, NullaryCallback> g_early_init_callbacks;

    Thread *g_game_thread;

    static bool g_engine_stopping = false;

    extern EngineConfig g_engine_config;

    static void _interrupt_handler(int signal) {
        stop_engine();
    }

    void kill_game_thread(void) {
        g_game_thread->detach();
        g_game_thread->destroy();
    }

    const Index register_update_callback(const DeltaCallback callback) {
        _ARGUS_ASSERT(g_core_initializing || g_core_initialized,
                "Cannot register update callback before engine initialization.");
        return add_callback(g_update_callbacks, callback);
    }

    void unregister_update_callback(const Index id) {
        remove_callback(g_update_callbacks, id);
    }

    const Index register_render_callback(const DeltaCallback callback) {
        _ARGUS_ASSERT(g_core_initializing || g_core_initialized,
                "Cannot register render callback before engine initialization.");
        return add_callback(g_render_callbacks, callback);
    }

    void unregister_render_callback(const Index id) {
        remove_callback(g_render_callbacks, id);
    }

    void register_early_init_callback(const std::string module_id, NullaryCallback callback) {
        g_early_init_callbacks.insert({ module_id, callback });
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

    static void *_game_loop(void *const _) {
        static Timestamp last_update = 0;

        while (1) {
            if (g_engine_stopping) {
                deinit_loaded_modules();
                break;
            }

            Timestamp update_start = argus::microtime();
            TimeDelta delta = _compute_delta(last_update);

            //TODO: should we flush the queues before the engine stops?
            flush_callback_list_queues(g_update_callbacks);
            flush_event_listener_queues(TargetThread::UPDATE);

            // invoke update callbacks
            g_update_callbacks.list_mutex.lock_shared();
            for (auto &callback : g_update_callbacks.list) {
                callback.value(delta);
            }
            g_update_callbacks.list_mutex.unlock_shared();

            process_event_queue(TargetThread::UPDATE);

            if (g_engine_config.target_tickrate != 0) {
                _handle_idle(update_start, g_engine_config.target_tickrate);
            }
        }

        return nullptr;
    }

    static void _render_loop() {
        static Timestamp last_frame = 0;

        while (1) {
            if (g_engine_stopping) {
                break;
            }


            Timestamp render_start = argus::microtime();
            TimeDelta delta = _compute_delta(last_frame);

            flush_callback_list_queues(g_render_callbacks);
            flush_event_listener_queues(TargetThread::RENDER);

            // invoke render callbacks
            g_render_callbacks.list_mutex.lock_shared();
            for (auto &callback : g_render_callbacks.list) {
                callback.value(delta);
            }
            g_render_callbacks.list_mutex.unlock_shared();

            process_event_queue(TargetThread::RENDER);

            if (g_engine_config.target_framerate != 0) {
                _handle_idle(render_start, g_engine_config.target_framerate);
            }
        }
    }

    void initialize_engine() {
        signal(SIGINT, _interrupt_handler);

        init_stock_modules();

        load_external_modules();

        if (g_engine_config.load_modules.size() > 0) {
            load_modules(g_engine_config.load_modules);
        } else {
            load_modules({"core"});
        }

        // this is basically for the sole purpose of allowing dynamic module
        // loading, e.g. allowing render to load render_opengl before any real
        // lifecycle stages are executed
        for (auto module : g_enabled_modules) {
            auto ei_callback = g_early_init_callbacks.find(module.id);
            if (ei_callback != g_early_init_callbacks.end()) {
                ei_callback->second();
            }
        }

        for (LifecycleStage stage = LifecycleStage::PRE_INIT; stage <= LifecycleStage::POST_INIT;
             stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            for (auto it = g_enabled_modules.cbegin(); it != g_enabled_modules.cend(); it++) {
                it->lifecycle_update_callback(stage);
            }
        }
    }

    void start_engine(const DeltaCallback game_loop) {
        _ARGUS_ASSERT(g_core_initialized, "Cannot start engine before it is initialized.");
        _ARGUS_ASSERT(game_loop != NULL, "start_engine invoked with null callback");

        register_update_callback(game_loop);

        g_game_thread = &Thread::create(_game_loop, nullptr);

        // pass control over to the render loop
        _render_loop();

        exit(0);
    }

    void stop_engine(void) {
        _ARGUS_ASSERT(g_core_initialized, "Cannot stop engine before it is initialized.");

        g_engine_stopping = true;
    }
}
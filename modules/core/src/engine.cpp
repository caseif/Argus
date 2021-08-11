/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/threading.hpp"
#include "argus/lowlevel/time.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core/callback.hpp"
#include "argus/core/engine.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/engine.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/event.hpp"
#include "internal/core/module.hpp"
#include "internal/core/module_core.hpp"

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <csignal>
#include <cstdint>
#include <cstdlib>

#define NS_PER_US 1000ULL

namespace argus {
    /**
     * \brief Represents an instant in time.
     */
    typedef uint64_t Timestamp;

    static CallbackList<DeltaCallback> g_update_callbacks;
    static CallbackList<DeltaCallback> g_render_callbacks;

    std::map<std::string, NullaryCallback> g_early_init_callbacks;

    Thread *g_game_thread;

    static bool g_engine_stopping = false;
    static std::mutex g_engine_stop_mutex;
    static std::condition_variable g_engine_stop_notifier;
    static std::atomic_bool g_render_thread_halted;

    extern EngineConfig g_engine_config;

    static void _interrupt_handler(const int signal) {
        UNUSED(signal);
        stop_engine();
    }

    void kill_game_thread(void) {
        g_game_thread->detach();
        g_game_thread->destroy();
    }

    Index register_update_callback(const DeltaCallback &callback) {
        _ARGUS_ASSERT(g_core_initializing || g_core_initialized,
                "Cannot register update callback before engine initialization.");
        return add_callback(g_update_callbacks, callback);
    }

    void unregister_update_callback(const Index id) {
        remove_callback(g_update_callbacks, id);
    }

    Index register_render_callback(const DeltaCallback &callback) {
        _ARGUS_ASSERT(g_core_initializing || g_core_initialized,
                "Cannot register render callback before engine initialization.");
        return add_callback(g_render_callbacks, callback);
    }

    void unregister_render_callback(const Index id) {
        remove_callback(g_render_callbacks, id);
    }

    void register_early_init_callback(const std::string &module_id, NullaryCallback callback) {
        g_early_init_callbacks.insert({ module_id, callback });
    }

    static void _handle_idle(Timestamp start_timestamp, unsigned int target_rate) {
        if (target_rate == 0) {
            return;
        }

        TimeDelta delta = argus::microtime() - start_timestamp;

        unsigned int frametime_target_us = US_PER_S / target_rate;
        if (delta < frametime_target_us) {
            uint64_t sleep_time_ns = (frametime_target_us - delta) * NS_PER_US;
            if (sleep_time_ns <= SLEEP_OVERHEAD_NS) {
                return;
            }
            sleep_nanos(sleep_time_ns - SLEEP_OVERHEAD_NS);
        }
    }

    static TimeDelta _compute_delta(Timestamp &last_timestamp) {
        TimeDelta delta = 0;

        if (last_timestamp != 0) {
            delta = argus::microtime() - last_timestamp;
        } else {
            delta = 0;
        }
        last_timestamp = argus::microtime();

        return delta;
    }

    static void _game_loop() {
        static Timestamp last_update = 0;

        while (true) {
            if (g_engine_stopping) {
                // wait for render thread to finish up what it's doing so we don't interrupt it and cause a segfault
                if (!g_render_thread_halted) {
                    _ARGUS_DEBUG("Render thread not halted, waiting\n");
                    std::unique_lock<std::mutex> lock(g_engine_stop_mutex);
                    g_engine_stop_notifier.wait(lock);
                }
                _ARGUS_DEBUG("Render thread is halted, proceeding with engine bring-down\n");

                deinit_loaded_modules();
                break;
            }

            Timestamp update_start = argus::microtime();
            TimeDelta delta = _compute_delta(last_update);

            //TODO: do we need to flush the queues before the engine stops?
            flush_callback_list_queues(g_update_callbacks);
            flush_event_listener_queues(TargetThread::Update);

            // invoke update callbacks
            g_update_callbacks.list_mutex.lock_shared();
            for (const auto &callback : g_update_callbacks.list) {
                callback.value(delta);
            }
            g_update_callbacks.list_mutex.unlock_shared();

            process_event_queue(TargetThread::Update);

            if (g_engine_config.target_tickrate != 0) {
                _handle_idle(update_start, g_engine_config.target_tickrate);
            }
        }
    }

    static void *_render_loop(void *const user_data) {
        UNUSED(user_data);

        static Timestamp last_frame = 0;

        while (true) {
            if (g_engine_stopping) {
                _ARGUS_DEBUG("Engine halt is acknowledged by render thread\n");
                std::unique_lock<std::mutex> lock(g_engine_stop_mutex);
                g_render_thread_halted = true;
                g_engine_stop_notifier.notify_one();
                break;
            }

            Timestamp render_start = argus::microtime();
            TimeDelta delta = _compute_delta(last_frame);

            flush_callback_list_queues(g_render_callbacks);
            flush_event_listener_queues(TargetThread::Render);

            // invoke render callbacks
            g_render_callbacks.list_mutex.lock_shared();
            for (const auto &callback : g_render_callbacks.list) {
                callback.value(delta);
            }
            g_render_callbacks.list_mutex.unlock_shared();

            process_event_queue(TargetThread::Render);

            if (g_engine_config.target_framerate != 0) {
                _handle_idle(render_start, g_engine_config.target_framerate);
            }
        }

        return nullptr;
    }

    void initialize_engine() {
        signal(SIGINT, _interrupt_handler);

        init_stock_modules();

        load_external_modules();

        if (!g_engine_config.load_modules.empty()) {
            load_modules(g_engine_config.load_modules);
        } else {
            load_modules({ ModuleCore });
        }

        // this is basically for the sole purpose of allowing dynamic module
        // loading, e.g. allowing render to load render_opengl before any real
        // lifecycle stages are executed
        for (const auto &mod_info : g_enabled_modules) {
            auto ei_callback = g_early_init_callbacks.find(mod_info.id);
            if (ei_callback != g_early_init_callbacks.end()) {
                ei_callback->second();
            }
        }

        for (LifecycleStage stage = LifecycleStage::PreInit; stage <= LifecycleStage::PostInit;
             stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            for (const auto &mod_info : g_enabled_modules) {
                mod_info.lifecycle_update_callback(stage);
            }
        }
    }

    void start_engine(const DeltaCallback &game_loop) {
        _ARGUS_ASSERT(g_core_initialized, "Cannot start engine before it is initialized.");
        _ARGUS_ASSERT(game_loop != NULL, "start_engine invoked with null callback");

        register_update_callback(game_loop);

        g_game_thread = &Thread::create(_render_loop, nullptr);

        // pass control over to the game loop
        _game_loop();

        exit(0);
    }

    void stop_engine(void) {
        _ARGUS_ASSERT(g_core_initialized, "Cannot stop engine before it is initialized.");

        g_engine_stopping = true;
    }
}

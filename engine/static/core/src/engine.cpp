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

// module lowlevel
#include "argus/lowlevel/atomic.hpp"
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
#include "internal/core/module_defs.hpp"

#include <atomic>
#include <chrono>
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

#define NS_PER_US 1'000ULL

namespace argus {
    static CallbackList<DeltaCallback> g_update_callbacks;
    static CallbackList<DeltaCallback> g_render_callbacks;

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

    static void _handle_idle(Timestamp start_timestamp, unsigned int target_rate) {
        using namespace std::chrono_literals;

        if (target_rate == 0) {
            return;
        }

        TimeDelta delta = argus::now() - start_timestamp;

        auto frametime_target = std::chrono::microseconds(US_PER_S / target_rate);
        if (delta < frametime_target) {
            std::chrono::nanoseconds sleep_time_ns = frametime_target - delta;
            if (sleep_time_ns <= SLEEP_OVERHEAD_NS) {
                return;
            }
            std::this_thread::sleep_for(sleep_time_ns - SLEEP_OVERHEAD_NS);
        }
    }

    static TimeDelta _compute_delta(Timestamp &last_timestamp) {
        using namespace std::chrono_literals;

        TimeDelta delta;

        if (last_timestamp.time_since_epoch() != 0s) {
            delta = argus::now() - last_timestamp;
        } else {
            delta = {};
        }
        last_timestamp = argus::now();

        return delta;
    }

    static void _game_loop() {
        static Timestamp last_update;

        while (true) {
            if (g_engine_stopping) {
                // wait for render thread to finish up what it's doing so we don't interrupt it and cause a segfault
                if (!g_render_thread_halted) {
                    _ARGUS_DEBUG("Render thread not halted, waiting");
                    std::unique_lock<std::mutex> lock(g_engine_stop_mutex);
                    g_engine_stop_notifier.wait(lock);
                }
                _ARGUS_DEBUG("Render thread is halted, proceeding with engine bring-down");

                deinit_modules();

                unload_dynamic_modules();

                break;
            }

            Timestamp update_start = argus::now();
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

        static Timestamp last_frame;

        while (true) {
            if (g_engine_stopping) {
                _ARGUS_DEBUG("Engine halt is acknowledged by render thread");
                std::unique_lock<std::mutex> lock(g_engine_stop_mutex);
                g_render_thread_halted = true;
                g_engine_stop_notifier.notify_one();
                break;
            }

            Timestamp render_start = argus::now();
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

        if (!g_engine_config.load_modules.empty()) {
            enable_modules(g_engine_config.load_modules);
        } else {
            enable_modules({ ModuleCore });
        }

        //load_dynamic_modules();

        init_modules();
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

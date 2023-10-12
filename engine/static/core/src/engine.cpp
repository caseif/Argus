/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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
#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/message.hpp"
#include "argus/lowlevel/threading.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/core/callback.hpp"
#include "argus/core/client_properties.hpp"
#include "argus/core/engine.hpp"
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"
#include "internal/core/callback_util.hpp"
#include "internal/core/engine.hpp"
#include "internal/core/engine_config.hpp"
#include "internal/core/event.hpp"
#include "internal/core/message.hpp"
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
#include <thread>
#include <utility>
#include <vector>

#include <csignal>
#include <cstdint>
#include <cstdlib>

#define NS_PER_US 1'000ULL

namespace argus {
    LifecycleStage g_cur_lifecycle_stage;

    static CallbackList<DeltaCallback> g_update_callbacks;
    static CallbackList<DeltaCallback> g_render_callbacks;

    static std::mutex g_one_off_callbacks_mutex;
    static std::vector<NullaryCallback> g_one_off_callbacks;

    static std::thread g_render_thread;
    static std::thread::id g_update_thread_id;

    static bool g_engine_stopping = false;
    static bool g_render_thread_acknowledged_halt = false;
    static bool g_force_shutdown_on_next_interrupt = false;
    static std::mutex g_engine_stop_mutex;
    static std::condition_variable g_engine_stop_notifier;
    static std::atomic_bool g_render_thread_halted;

    static void _interrupt_handler(const int signal) {
        UNUSED(signal);
        stop_engine();
    }

    void kill_render_thread(void) {
        g_render_thread.detach();
    }

    Index register_update_callback(const DeltaCallback &callback, Ordering ordering) {
        affirm_precond(g_core_initializing || g_core_initialized,
                "Cannot register update callback before engine initialization.");
        return add_callback(g_update_callbacks, callback, ordering);
    }

    void unregister_update_callback(const Index id) {
        remove_callback(g_update_callbacks, id);
    }

    Index register_render_callback(const DeltaCallback &callback, Ordering ordering) {
        affirm_precond(g_core_initializing || g_core_initialized,
                "Cannot register render callback before engine initialization.");
        return add_callback(g_render_callbacks, callback, ordering);
    }

    void unregister_render_callback(const Index id) {
        remove_callback(g_render_callbacks, id);
    }

    static void _deinit_callbacks(void) {
        g_update_callbacks.lists.clear();
        g_render_callbacks.lists.clear();
    }

    void run_on_game_thread(NullaryCallback callback) {
        g_one_off_callbacks_mutex.lock();
        g_one_off_callbacks.push_back(callback);
        g_one_off_callbacks_mutex.unlock();
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

    static void _deinit_engine() {
        Logger::default_logger().debug("Engine halt request is acknowledged game thread");
        g_render_thread_acknowledged_halt = true;

        // wait for render thread to finish up what it's doing so we don't interrupt it and cause a segfault
        if (!g_render_thread_halted) {
            Logger::default_logger().debug(
                    "Game thread observed render thread was not halted, waiting on monitor (send SIGINT again to force halt)");
            std::unique_lock<std::mutex> lock(g_engine_stop_mutex);
            g_engine_stop_notifier.wait(lock);
        }

        // at this point all event and callback execution should have
        // stopped which allows us to start doing non-thread-safe things

        Logger::default_logger().debug(
                "Game thread observed render thread is halted, proceeding with engine bring-down");

        // unregister message dispatcher to avoid static deinitialization order fiasco
        set_message_dispatcher(nullptr);

        Logger::default_logger().debug("Deinitializing engine modules");

        deinit_modules();

        Logger::default_logger().debug("Deinitializing event callbacks");

        // if we don't do this explicitly, the callback lists (and thus
        // the callback function objects) will be deinitialized
        // statically and will segfault on handlers registered by
        // external libraries (which will have already been unloaded)
        deinit_event_handlers();

        Logger::default_logger().debug("Deinitializing general callbacks");

        // same deal here
        _deinit_callbacks();

        Logger::default_logger().debug("Unloading dynamic engine modules");

        unload_dynamic_modules();

        Logger::default_logger().info("Engine bring-down completed");
    }

    static void _game_loop() {
        static Timestamp last_update;

        while (true) {
            if (g_engine_stopping) {
                _deinit_engine();
                break;
            }

            Timestamp update_start = argus::now();
            TimeDelta delta = _compute_delta(last_update);

            // prioritize one-off callbacks
            g_one_off_callbacks_mutex.lock();
            for (auto callback : g_one_off_callbacks) {
                callback();
            }
            g_one_off_callbacks.clear();
            g_one_off_callbacks_mutex.unlock();

            //TODO: do we need to flush the queues before the engine stops?
            flush_callback_list_queues(g_update_callbacks);
            flush_event_listener_queues(TargetThread::Update);

            // invoke update callbacks
            g_update_callbacks.list_mutex.lock_shared();
            for (auto ordering : ORDERINGS) {
                for (const auto &callback : g_update_callbacks.lists[ordering]) {
                    callback.value(delta);
                }
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
                Logger::default_logger().debug("Engine halt request is acknowledged by render thread");
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
            for (auto ordering : ORDERINGS) {
                for (const auto &callback : g_render_callbacks.lists[ordering]) {
                    callback.value(delta);
                }
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
        Logger::default_logger().info("Engine initialization started");

        signal(SIGINT, _interrupt_handler);

        g_update_thread_id = std::this_thread::get_id();

        set_message_dispatcher(dispatch_message);

        Logger::default_logger().debug("Enabling requested modules");
        if (!g_engine_config.load_modules.empty()) {
            enable_modules(g_engine_config.load_modules);
        } else {
            enable_modules({ModuleCore});
        }

        //load_dynamic_modules();

        Logger::default_logger().debug("Initializing enabled modules");

        init_modules();

        Logger::default_logger().info("Engine initialized!");
    }

    [[noreturn]] void start_engine(const DeltaCallback &game_loop) {
        Logger::default_logger().info("Bringing up engine");

        affirm_precond(g_core_initialized, "Cannot start engine before it is initialized.");
        affirm_precond(game_loop != nullptr, "start_engine invoked with null callback");

        affirm_precond(!get_client_id().empty(), "Client ID must be set prior to engine start");
        affirm_precond(!get_client_name().empty(), "Client ID must be set prior to engine start");
        affirm_precond(!get_client_version().empty(), "Client ID must be set prior to engine start");

        register_update_callback(game_loop);

        g_render_thread = std::thread(_render_loop, nullptr);

        Logger::default_logger().info("Engine started! Passing control to game loop.");

        // pass control over to the game loop
        _game_loop();

        Logger::default_logger().info("Game loop has halted, exiting program");

        exit(0);
    }

    void stop_engine(void) {
        if (g_force_shutdown_on_next_interrupt) {
            Logger::default_logger().info("Forcibly terminating process");
            std::exit(1);
        } else if (g_render_thread_acknowledged_halt) {
            Logger::default_logger().info("Forcibly proceeding with engine bring-down");
            g_force_shutdown_on_next_interrupt = true;
            g_engine_stop_notifier.notify_one();
        } else if (g_engine_stopping) {
            Logger::default_logger().warn("Engine is already halting");
        }

        Logger::default_logger().info("Engine halt requested");

        affirm_precond(g_core_initialized, "Cannot stop engine before it is initialized.");

        g_engine_stopping = true;
    }

    LifecycleStage get_current_lifecycle_stage(void) {
        return g_cur_lifecycle_stage;
    }

    bool is_current_thread_update_thread(void) {
        return std::this_thread::get_id() == g_update_thread_id;
    }
}

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

#pragma once

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"

#include "argus/core/callback.hpp"
#include "argus/core/module.hpp"

#include <functional>
#include <initializer_list>
#include <vector>

namespace argus {
    /**
     * @brief An update callback accepts a single parameter representing the
     *        delta in microseconds since the last update.
     */
    typedef std::function<void(const TimeDelta)> DeltaCallback;

    /**
     * @brief A callback accepts no parameters and returns void.
     */
    typedef std::function<void(void)> NullaryCallback;

    enum class Ordering {
        First,
        Early,
        Standard,
        Late,
        Last
    };

    constexpr Ordering ORDERINGS[] = {
            Ordering::First,
            Ordering::Early,
            Ordering::Standard,
            Ordering::Late,
            Ordering::Last
    };

    /**
     * @brief Initializes the engine.
     *
     * argus::set_load_modules(const std::initializer_list) should be invoked
     * before this function is called. If the load modules have not been
     * configured, only the `core` module will be loaded.
     *
     * @attention This must be called before any other interaction with the
     * engine takes place.
     */
    void initialize_engine(void);

    /**
     * @brief Starts the engine.
     *
     * @param game_loop The callback representing the main game loop.
     */
    [[noreturn]] void start_engine(const DeltaCallback &game_loop);

    /**
     * @brief Requests that the engine halt execution, performing cleanup as
     *        necessary.
     */
    void stop_engine(void);

    /**
     * @brief Crashes the engine with the given error message.
     *
     * This function is intended for internal engine use only.
     *
     * @param format The message to display.
     */
    [[noreturn]] void _crash_va(const char *format, ...);

    /**
     * @brief Crashes the engine with the given error message.
     *
     * The message will be printed to stdout and may be displayed to the end
     * user in some form, but this is not guaranteed.
     *
     * @param format The message to display.
     */
     template<typename... Args>
     [[noreturn]] inline void crash(const std::string &format, Args... args) {
        _crash_va(format.c_str(), args...);
     }

    /**
     * @brief Crashes the engine with the given error message.
     *
     * The message will be printed to stderr and may be displayed to the end
     * user in some form, but this is not guaranteed.
     *
     * @param format The message to display.
     * @param args The format arguments for the message.
     */
    [[noreturn]] void crash(const char *format, va_list args);

    /**
     * @brief Gets the current lifecycle stage of the engine.
     *
     * @return The current lifecycle stage of the engine.
     */
    LifecycleStage get_current_lifecycle_stage(void);

    /**
     * @brief Registers a callback for invocation on each game update.
     *
     * It is normally not necessary to invoke this from game code.
     *
     * @param update_callback The callback to be invoked on each update.
     *
     * @return The ID of the new registration.
     *
     * @sa DeltaCallback
     */
    Index register_update_callback(const DeltaCallback &update_callback, Ordering ordering = Ordering::Standard);

    /**
     * @brief Unregisters an update callback.
     *
     * @param id The ID of the callback to unregister.
     */
    void unregister_update_callback(Index id);

    /**
     * @brief Registers a callback for invocation on each render update.
     *
     * It is normally not necessary to invoke this from game code.
     *
     * @param render_callback The callback to be invoked on each frame.
     *
     * @return The ID of the new registration.
     *
     * @sa DeltaCallback
     */
    Index register_render_callback(const DeltaCallback &render_callback, Ordering ordering = Ordering::Standard);

    /**
     * @brief Unregisters a render callback.
     *
     * @param id The ID of the callback to unregister.
     */
    void unregister_render_callback(Index id);

    /**
     * @brief Invokes a callback on the game thread during the next tick.
     *
     * @param callback The callback to invoke.
     */
    void run_on_game_thread(const NullaryCallback &callback);

    bool is_current_thread_update_thread(void);
}

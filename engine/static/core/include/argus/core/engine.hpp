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
     * \brief An update callback accepts a single parameter representing the
     *        delta in microseconds since the last update.
     */
    typedef std::function<void(const TimeDelta)> DeltaCallback;

    /**
     * \brief A callback accepts no parameters and returns void.
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
     * \brief Initializes the engine.
     *
     * argus::set_load_modules(const std::initializer_list) should be invoked
     * before this function is called. If the load modules have not been
     * configured, only the `core` module will be loaded.
     *
     * \attention This must be called before any other interaction with the
     * engine takes place.
     *
     * \throw std::invalid_argument If any of the requested modules (or their
     *        dependencies) cannot be loaded.
     */
    void initialize_engine(void);

    /**
     * \brief Starts the engine.
     *
     * \param game_loop The callback representing the main game loop.
     */
    [[noreturn]] void start_engine(const DeltaCallback &game_loop);

    /**
     * \brief Requests that the engine halt execution, performing cleanup as
     *        necessary.
     */
    void stop_engine(void);

    /**
     * \brief Gets the current lifecycle stage of the engine.
     *
     * \return The current lifecycle stage of the engine.
     */
     LifecycleStage get_current_lifecycle_stage(void);

    /**
     * \brief Registers a callback for invocation on each game update.
     *
     * It is normally not necessary to invoke this from game code.
     *
     * \param update_callback The callback to be invoked on each update.
     *
     * \return The ID of the new registration.
     *
     * \sa DeltaCallback
     */
    Index register_update_callback(const DeltaCallback &update_callback, Ordering ordering = Ordering::Standard);

    /**
     * \brief Unregisters an update callback.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_update_callback(Index id);

    /**
     * \brief Registers a callback for invocation on each render update.
     *
     * It is normally not necessary to invoke this from game code.
     *
     * \param render_callback The callback to be invoked on each frame.
     *
     * \return The ID of the new registration.
     *
     * \sa DeltaCallback
     */
    Index register_render_callback(const DeltaCallback &render_callback, Ordering ordering = Ordering::Standard);

    /**
     * \brief Unregisters a render callback.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_render_callback(Index id);

    /**
     * \brief Invokes a callback on the game thread during the next tick.
     *
     * \param callback The callback to invoke.
     */
    void run_on_game_thread(NullaryCallback callback);

    bool is_current_thread_update_thread(void);
}

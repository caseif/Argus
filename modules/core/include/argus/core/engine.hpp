/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/time.hpp"

// module core
#include "argus/core/callback.hpp"

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
    void start_engine(const DeltaCallback game_loop);

    /**
     * \brief Requests that the engine halt execution, performing cleanup as
     *        necessary.
     */
    void stop_engine(void);

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
    const Index register_update_callback(const DeltaCallback update_callback);

    /**
     * \brief Unregisters an update callback.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_update_callback(const Index id);

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
    const Index register_render_callback(const DeltaCallback render_callback);

    /**
     * \brief Unregisters a render callback.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_render_callback(const Index id);
}

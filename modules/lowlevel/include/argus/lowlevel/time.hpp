/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

/**
 * \file argus/lowlevel/time.hpp
 *
 * Time-related utility functions.
 */

#pragma once

#include <chrono>

#include <cstdint>

namespace argus {
    /**
     * \brief Represents an instant in time.
     */
    typedef std::chrono::time_point<std::chrono::steady_clock> Timestamp;

    /**
     * \brief Represents a duration of time.
     */
    typedef std::chrono::nanoseconds TimeDelta;

    /**
     * \brief Returns the current monotonic time.
     *
     * \return The current monotonic time.
     */
    Timestamp now(void);
}

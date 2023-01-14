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

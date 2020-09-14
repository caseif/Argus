/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

/**
 * \file argus/time.hpp
 *
 * Time-related utility functions.
 */

#pragma once

namespace argus {

    /**
     * \brief Sleeps for the specified amount of time in nanoseconds.
     *
     * \param ns The number of nanoseconds to sleep for.
     */
    void sleep_nanos(const unsigned long long ns);

    /**
     * \brief Returns the number of microseconds since the Unix epoch.
     *
     * \return The number of microseconds since the Unix epoch.
     */
    const unsigned long long microtime(void);

}

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

/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/time.hpp"

#include <chrono>
#include <thread>
#ifdef _WIN32
    #include <Windows.h>
#else
    #include <sys/time.h>

    #include <ctime>
    #ifdef USE_PTHREADS
        #include <unistd.h>
    #endif
#endif

#define NS_PER_S 1000000000LLU

namespace argus {

#ifdef _WIN32
    #define WIN32_EPOCH_OFFSET 11644473600000000ULL

    const unsigned long long microtime(void) {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        unsigned long long tt = ft.dwHighDateTime;
        tt <<= 32;
        tt |= ft.dwLowDateTime;
        tt /= 10;
        tt -= WIN32_EPOCH_OFFSET;
        return tt;
    }
#else
    const unsigned long long microtime(void) {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec * 1000000 + now.tv_nsec / 1000;
    }
#endif

#ifdef USE_PTHREADS
    void sleep_nanos(const unsigned long long ns) {
        const struct timespec spec = {(long)(ns / NS_PER_S), (long)(ns % NS_PER_S)};
        nanosleep(&spec, NULL);
    }
#else
    void sleep_nanos(const unsigned long long ns) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
    }
#endif

}

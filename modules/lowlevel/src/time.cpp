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

#include <cstdint>

#define NS_PER_S 1000000000LLU
#define US_PER_S 1000000LLU
#define NS_PER_US 1000LLU

namespace argus {

#ifdef _WIN32
    #define WIN32_EPOCH_OFFSET 11644473600000000ULL

    uint64_t microtime(void) {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        uint64_t tt = ft.dwHighDateTime;
        tt <<= 32;
        tt |= ft.dwLowDateTime;
        tt /= 10;
        tt -= WIN32_EPOCH_OFFSET;
        return tt;
    }
#else
    uint64_t microtime(void) {
        timespec now{};
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec * US_PER_S + now.tv_nsec / NS_PER_US;
    }
#endif

#ifdef USE_PTHREADS
    void sleep_nanos(const uint64_t ns) {
        const struct timespec spec = {(long)(ns / NS_PER_S), (long)(ns % NS_PER_S)};
        nanosleep(&spec, NULL);
    }
#else
    void sleep_nanos(const uint64_t ns) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
    }
#endif

}

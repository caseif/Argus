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
#endif

#include <cstdint>

#define NS_PER_S 1'000'000'000ULL

namespace argus {
    Timestamp now(void) {
        return std::chrono::steady_clock::now();
    }
}

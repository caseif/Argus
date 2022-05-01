/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

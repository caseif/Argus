/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include <cstdio>
#include <cstdlib>

#ifdef _ARGUS_DEBUG_MODE
#ifdef _MSC_VER
#include <intrin.h>
#define DEBUG_BREAK() __debugbreak()
#else
#include <csignal>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif
#else
#define DEBUG_BREAK()
#endif

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#ifdef _ARGUS_DEBUG_MODE
#define _GENERIC_PRINT(stream, level, system, fmt, ...) fprintf(stream, "[%s][%s] " __FILE__ ":" STRINGIZE(__LINE__) ": " fmt, level, system, ##__VA_ARGS__)
#else
#define _GENERIC_PRINT(stream, level, system, fmt, ...) fprintf(stream, "[%s][%s] " fmt, level, system, ##__VA_ARGS__)
#endif

#define _ARGUS_PRINT(stream, level, fmt, ...) _GENERIC_PRINT(stream, level, "Argus", fmt, ##__VA_ARGS__)

#ifdef _ARGUS_DEBUG_MODE
#define _ARGUS_DEBUG(fmt, ...) _ARGUS_PRINT(stdout, "DEBUG", fmt, ##__VA_ARGS__)
#else
#define _ARGUS_DEBUG(fmt, ...)
#endif

#define _ARGUS_INFO(fmt, ...) _ARGUS_PRINT(stdout, "INFO", fmt, ##__VA_ARGS__)
#define _ARGUS_WARN(fmt, ...) _ARGUS_PRINT(stderr, "WARN", fmt, ##__VA_ARGS__)
#define _ARGUS_FATAL_SOFT(fmt, ...) 

#define _ARGUS_ABORT()          DEBUG_BREAK(); \
                                exit(1)

#define _ARGUS_FATAL(fmt, ...)  _ARGUS_PRINT(stderr, "FATAL", fmt, ##__VA_ARGS__); \
                                _ARGUS_ABORT();

#define _ARGUS_FATAL_DEINIT(deinit_body, fmt, ...)  _ARGUS_PRINT(stderr, "FATAL", fmt, ##__VA_ARGS__); \
                                                    deinit_body; \
                                                    _ARGUS_ABORT();

#define _ARGUS_ASSERT(c, fmt, ...)  if (!(c)) { \
                                        _ARGUS_FATAL(fmt, ##__VA_ARGS__); \
                                    }

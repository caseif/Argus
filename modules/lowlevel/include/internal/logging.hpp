/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

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

#define _ARGUS_FATAL(fmt, ...)  _ARGUS_PRINT(stderr, "FATAL", fmt, ##__VA_ARGS__); \
                                exit(1)

#define _ARGUS_ASSERT(c, fmt, ...)  if (!(c)) {     \
                                        _ARGUS_FATAL(fmt, ##__VA_ARGS__);   \
                                    }

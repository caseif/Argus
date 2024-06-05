/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/crash.hpp"
#include "argus/lowlevel/logging.hpp"
#include "internal/lowlevel/crash.hpp"

#include <cstdarg>

namespace argus {
    static constexpr const char *g_log_level_fatal = "FATAL";

    static CrashCallback g_ll_crash_callback = nullptr;

    void set_ll_crash_callback(CrashCallback callback) {
        g_ll_crash_callback = callback;
    }

    [[noreturn]] static void _crash_ll_fallback(const char *format, va_list args) {
        Logger::default_logger().log_error(g_log_level_fatal, format, args);
        exit(1);
    }

    [[noreturn]] void _crash_ll_va(const char *format, ...) {
        va_list args;
        va_start(args, format);
        if (g_ll_crash_callback != nullptr) {
            g_ll_crash_callback(format, args);
            // the crash callback will implicitly exit, but it's not possible
            // within the C++ standard to specify that a pointed-to function
            // does not return so we have to add an unreachable exit call here
            exit(1);
        } else {
            _crash_ll_fallback(format, args);
        }
        va_end(args);
    }
}

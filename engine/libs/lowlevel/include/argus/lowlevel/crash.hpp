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

#pragma once

#include <functional>
#include <string>

namespace argus {
    typedef void(*CrashCallback)(const char *, va_list);

    void set_ll_crash_callback(CrashCallback callback);

    [[noreturn]] void _crash_ll_va(const char *format, ...);

    template<typename... Args>
    [[noreturn]] void crash_ll(const std::string &format, Args... args) {
        _crash_ll_va(format.c_str(), args...);
    }
}

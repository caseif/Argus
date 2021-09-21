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

#include <stdexcept>
#include <system_error>

#ifdef _WIN32
    #include <Windows.h>

    #ifndef errno
    #define errno WSAGetLastError()
    #endif
#endif

#define validate_arg(cond, what) _validate_arg(cond, __func__, what)
#define validate_arg_not(cond, what) validate_arg(!(cond), what)
#define validate_state(cond, what) _validate_state(cond, __func__, what)
#define validate_state_not(cond, what) validate_state(!(cond), what)
#define throw_errno(syscall) _throw_errno(__func__, syscall)
#define validate_syscall(cond, syscall) _validate_syscall(cond, __func__, syscall)

inline void _validate_arg(bool cond, const std::string &caller, const std::string &what) {
    if (!cond) {
        throw std::invalid_argument(caller + ": " + what);
    }
}

inline void _validate_state(bool cond, const std::string &caller, const std::string &what) {
    if (!cond) {
        throw std::runtime_error(caller + ": Invalid state: " + what);
    }
}

inline void _throw_errno(const std::string &caller, const std::string &syscall) {
    throw std::system_error(errno, std::generic_category(), caller + ": " + syscall + " failed");
}

inline void _validate_syscall(bool cond, const std::string &caller, const std::string &syscall) {
    if (!cond) {
        _throw_errno(caller, syscall);
    }
}

inline void _validate_syscall(int rc, const std::string &caller, const std::string &syscall) {
    _validate_syscall(rc == 0, caller, syscall);
}

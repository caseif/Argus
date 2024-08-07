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

#include "argus/lowlevel/crash.hpp"
#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"

#ifdef _ARGUS_DEBUG_MODE
#define _affirm_precond(cond, fmt, file, line)  ((cond) \
        ? void(0) \
        : ::argus::crash("Precondition failed: " file ":" STRINGIZE(line) ": " #cond " (" fmt ")"))
#define affirm_precond(cond, fmt)  _affirm_precond((cond), fmt, __FILE__, __LINE__)
#else
#define affirm_precond(cond, fmt)  ((cond) \
        ? void(0) \
        : argus::crash("Precondition failed: " #cond " (" fmt ")"))
#endif

#ifdef _ARGUS_DEBUG_MODE
#define _argus_assert(cond, file, line) ((cond) \
        ? void(0) \
        : ::argus::crash("Assertion failed: " file ":" STRINGIZE(line) ": " #cond))
#define argus_assert(...) _argus_assert((__VA_ARGS__), __FILE__, __LINE__)

#define _argus_assert_ll(cond, file, line) ((cond) \
        ? void(0) \
        : ::argus::crash_ll("Assertion failed: " file ":" STRINGIZE(line) ": " #cond))
#define argus_assert_ll(...) _argus_assert_ll((__VA_ARGS__), __FILE__, __LINE__)
#else
#define _argus_assert(cond) ((cond) ? (void(0)) : (argus::crash("Assertion failed: " #cond)))
#define argus_assert(...) _argus_assert((__VA_ARGS__))
#define _argus_assert_ll(cond) ((cond) ? (void(0)) : (argus::crash_ll("Assertion failed: " #cond)))
#define argus_assert_ll(...) _argus_assert_ll((__VA_ARGS__))
#endif

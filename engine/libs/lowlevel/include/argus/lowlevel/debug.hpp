/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/logging.hpp"

#define affirm_precond(cond, fmt)  (cond ? void(0) : Logger::default_logger().fatal("Precondition failed: " #cond " (" fmt ")"))

#ifdef assert
#undef assert
#endif
#ifdef _ARGUS_DEBUG_MODE
#define _assert(cond, file, line) ((cond) ? void(0) : Logger::default_logger().fatal("Assertion failed: " #cond " (" file ":" #line ")"))
#define assert(cond) _assert(cond, __FILE__, __LINE__)
#else
#define assert(cond) ((cond) ? (void(0)) : (Logger::default_logger().fatal("Assertion failed: " #cond)))
#endif

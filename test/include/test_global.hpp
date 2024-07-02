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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wparentheses"

#define CATCH_CONFIG_MAIN
#include "catch2/catch_template_test_macros.hpp"
#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "catch2/generators/catch_generators_adapters.hpp"
#include "catch2/generators/catch_generators_random.hpp"
#include "catch2/generators/catch_generators_range.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"

#pragma GCC diagnostic pop

#include <csetjmp>

extern std::jmp_buf g_jmpbuf;

#define REQUIRE_CRASHES(expr) \
    if (setjmp(g_jmpbuf) == 0) { \
        (void)(expr); \
        FAIL("expression did not trigger crash"); \
    } else { \
        SUCCEED("expression triggered crash"); \
    }

#define REQUIRE_NOCRASH(expr) \
    if (setjmp(g_jmpbuf) == 0) { \
        (void)(expr); \
        SUCCEED("expression did not trigger crash"); \
    } else { \
        FAIL("expression triggered crash"); \
    }

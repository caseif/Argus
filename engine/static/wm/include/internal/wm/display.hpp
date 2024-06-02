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

#include "argus/wm/cabi/display.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include "SDL_video.h"

#pragma GCC diagnostic pop

namespace argus {
    // forward declarations
    class Display;

    void init_display(void);

    const Display *get_display_from_index(int index);

    DisplayMode wrap_display_mode(SDL_DisplayMode mode);

    SDL_DisplayMode unwrap_display_mode(const DisplayMode &mode);

    union DisplayModeUnion {
        argus_display_mode_t c_mode {};
        argus::DisplayMode cpp_mode;

        DisplayModeUnion() {}
    };

    inline argus_display_mode_t as_c_display_mode(argus::DisplayMode mode) {
        DisplayModeUnion u;
        u.cpp_mode = mode;
        return u.c_mode;
    }

    inline argus::DisplayMode as_cpp_display_mode(argus_display_mode_t mode) {
        DisplayModeUnion u;
        u.c_mode = mode;
        return u.cpp_mode;
    }
}

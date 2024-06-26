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

#include "argus/lowlevel/math.hpp"

#include "argus/wm/display.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include "SDL_video.h"

#pragma GCC diagnostic pop

#include <string>
#include <vector>

namespace argus {
    struct pimpl_Display {
        int index;

        std::string name;
        Vector2i position;

        std::vector<DisplayMode> modes;

        pimpl_Display(int index, std::string name, Vector2i position, std::vector<DisplayMode> modes):
            index(index),
            name(std::move(name)),
            position(position),
            modes(std::move(modes)) {
        }
    };
}

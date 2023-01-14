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

#include "argus/lowlevel/math.hpp"

#include <string>
#include <vector>

#include <cstdint>

namespace argus {
    // forward declarations
    struct pimpl_Display;

    struct DisplayMode {
        Vector2u resolution;
        uint16_t refresh_rate;
        Vector3u color_depth;
    };

    class Display {
        private:
            Display(Display&) = delete;

            Display(Display&&) = delete;

        public:
            pimpl_Display *pimpl;

            Display(void);

            ~Display(void);

            static const std::vector<const Display*> &get_available_displays(void);

            const std::string &get_name(void) const;

            const Vector2i &get_position(void) const;

            const Vector2u &get_physical_size(void) const;

            const Vector2f &get_scale(void) const;

            const std::vector<DisplayMode> &get_display_modes(void) const;
    };
}

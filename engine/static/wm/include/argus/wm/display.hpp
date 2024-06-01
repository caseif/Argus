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

#include "argus/scripting.hpp"

#include <string>
#include <vector>

#include <cstdint>

namespace argus {
    // forward declarations
    struct pimpl_Display;

    struct DisplayMode {
        Vector2u resolution;
        uint16_t refresh_rate;
        Vector4u color_depth;
        uint32_t extra_data;
    };

    class Display : AutoCleanupable {
      private:
        Display(Display &) = delete;

        Display(Display &&) = delete;

      public:
        pimpl_Display *m_pimpl;

        Display(void);

        ~Display(void) override;

        static const std::vector<const Display *> &get_available_displays(void);

        const std::string &get_name(void) const;

        Vector2i get_position(void) const;

        const std::vector<DisplayMode> &get_display_modes(void) const;
    };
}

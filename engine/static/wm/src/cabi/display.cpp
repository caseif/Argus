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

#include "argus/wm/display.hpp"
#include "argus/wm/cabi/display.h"

using argus::Display;

static const Display &_as_ref(argus_display_const_t ptr) {
    return *reinterpret_cast<const Display *>(ptr);
}

#ifdef __cplusplus
extern "C" {
#endif

void argus_display_get_available_displays(size_t *out_count, argus_display_const_t *out_displays) {
    auto &res = Display::get_available_displays();

    if (out_count != nullptr) {
        *out_count = res.size();
    }

    if (out_displays != nullptr) {
        for (size_t i = 0; i < res.size(); i++) {
            out_displays[i] = res[i];
        }
    }
}

const char *argus_display_get_name(argus_display_const_t self) {
    return _as_ref(self).get_name().c_str();
}

argus_vector_2i_t argus_display_get_position(argus_display_const_t self) {
    auto res = _as_ref(self).get_position();
    return *reinterpret_cast<argus_vector_2i_t *>(reinterpret_cast<void *>(&res));
}

void argus_display_get_display_modes(argus_display_const_t self, size_t *out_count, argus_display_mode_t *out_modes) {
    auto &res = _as_ref(self).get_display_modes();

    if (out_count != nullptr) {
        *out_count = res.size();
    }

    if (out_modes != nullptr) {
        for (size_t i = 0; i < res.size(); i++) {
            out_modes[i] = *reinterpret_cast<const argus_display_mode_t *>(reinterpret_cast<const void *>(&res[i]));
        }
    }
}

#ifdef __cplusplus
}
#endif

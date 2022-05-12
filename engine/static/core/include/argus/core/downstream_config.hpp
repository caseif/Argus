/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

namespace argus {
    struct InitialWindowParameters {
        std::string id;
        std::string title;
        std::string mode;
        bool vsync;
        bool mouse_visible;
        bool mouse_captured;
        Vector2i position;
        Vector2u dimensions;
    };

    const InitialWindowParameters &get_initial_window_parameters(void);

    void set_initial_window_parameters(const InitialWindowParameters &window_params);

    const std::string &get_default_bindings_resource_id(void);

    void set_default_bindings_resource_id(const std::string &resource_id);

    bool get_save_user_bindings(void);

    void set_save_user_bindings(bool save);
}

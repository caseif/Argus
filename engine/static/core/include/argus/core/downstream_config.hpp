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

#include <optional>
#include <string>

namespace argus {
    struct ScriptingParameters {
        std::optional<std::string> main;
    };

    struct InitialWindowParameters {
        std::optional<std::string> id;
        std::optional<std::string> title;
        std::optional<std::string> mode;
        std::optional<bool> vsync;
        std::optional<bool> mouse_visible;
        std::optional<bool> mouse_captured;
        std::optional<bool> mouse_raw_input;
        std::optional<Vector2i> position;
        std::optional<Vector2u> dimensions;
    };

    const ScriptingParameters &get_scripting_parameters(void);

    void set_scripting_parameters(const ScriptingParameters &params);

    const InitialWindowParameters &get_initial_window_parameters(void);

    void set_initial_window_parameters(const InitialWindowParameters &window_params);

    const std::string &get_default_bindings_resource_id(void);

    void set_default_bindings_resource_id(const std::string &resource_id);

    bool get_save_user_bindings(void);

    void set_save_user_bindings(bool save);
}

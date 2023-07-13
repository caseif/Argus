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

#include "argus/core/downstream_config.hpp"

#include <string>

namespace argus {
    static std::optional<ScriptingParameters> g_scripting_params;
    static std::optional<InitialWindowParameters> g_init_window_params;
    static std::string g_def_bindings_res_id;
    static bool g_save_user_bindings;

    const std::optional<ScriptingParameters> &get_scripting_parameters(void) {
        return g_scripting_params;
    }

    void set_scripting_parameters(const ScriptingParameters &params) {
        g_scripting_params = params;
    }

    const std::optional<InitialWindowParameters> &get_initial_window_parameters(void) {
        return g_init_window_params;
    }

    void set_initial_window_parameters(const InitialWindowParameters &window_params) {
        g_init_window_params = window_params;
    }

    const std::string &get_default_bindings_resource_id(void) {
        return g_def_bindings_res_id;
    }

    void set_default_bindings_resource_id(const std::string &resource_id) {
        g_def_bindings_res_id = resource_id;
    }

    bool get_save_user_bindings(void) {
        return g_save_user_bindings;
    }

    void set_save_user_bindings(bool save) {
        g_save_user_bindings = save;
    }
}

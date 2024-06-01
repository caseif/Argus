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

#include "argus/lowlevel/math/vector.hpp"

#include "argus/core/downstream_config.hpp"
#include "argus/core/cabi/downstream_config.h"

argus_scripting_parameters_t argus_get_scripting_parameters(void) {
    const auto &real_params = argus::get_scripting_parameters();

    argus_scripting_parameters_t params {};

    if ((params.has_main = real_params.main.has_value())) {
        params.main = real_params.main->c_str();
    }

    return params;
}

void argus_set_scripting_parameters(const argus_scripting_parameters_t *params) {
    argus::ScriptingParameters real_params {};

    if (params->has_main) {
        real_params.main = params->main;
    }

    argus::set_scripting_parameters(real_params);
}

argus_initial_window_parameters_t argus_get_initial_window_parameters(void) {
    const auto &real_params = argus::get_initial_window_parameters();

    argus_initial_window_parameters_t params {};

    if ((params.has_id = real_params.id.has_value())) {
        params.id = real_params.id->c_str();
    }
    if ((params.has_title = real_params.title.has_value())) {
        params.title = real_params.title->c_str();
    }
    if ((params.has_mode = real_params.mode.has_value())) {
        params.mode = real_params.mode->c_str();
    }
    if ((params.has_vsync = real_params.vsync.has_value())) {
        params.vsync = real_params.vsync.value();
    }
    if ((params.has_mouse_visible = real_params.mouse_visible.has_value())) {
        params.mouse_visible = real_params.mouse_visible.value();
    }
    if ((params.has_mouse_captured = real_params.mouse_captured.has_value())) {
        params.mouse_captured = real_params.mouse_captured.value();
    }
    if ((params.has_mouse_raw_input = real_params.mouse_raw_input.has_value())) {
        params.mouse_raw_input = real_params.mouse_raw_input.value();
    }
    if ((params.has_position = real_params.position.has_value())) {
        params.position = argus::as_c_vec(real_params.position.value());
    }
    if ((params.has_dimensions = real_params.dimensions.has_value())) {
        params.dimensions = argus::as_c_vec(real_params.dimensions.value());
    }

    return params;
}

void argus_set_initial_window_parameters(const argus_initial_window_parameters_t *params) {
    argus::InitialWindowParameters real_params {};

    if (params->has_id) {
        real_params.id = params->id;
    }
    if (params->has_title) {
        real_params.title = params->title;
    }
    if (params->has_mode) {
        real_params.mode = params->mode;
    }
    if (params->has_vsync) {
        real_params.vsync = params->vsync;
    }
    if (params->has_mouse_visible) {
        real_params.mouse_visible = params->mouse_visible;
    }
    if (params->has_mouse_captured) {
        real_params.mouse_captured = params->mouse_captured;
    }
    if (params->has_mouse_raw_input) {
        real_params.mouse_raw_input = params->mouse_raw_input;
    }
    if (params->has_position) {
        real_params.position = argus::as_cpp_vec(params->position);
    }
    if (params->has_dimensions) {
        real_params.dimensions = argus::as_cpp_vec(params->dimensions);
    }

    argus::set_initial_window_parameters(real_params);
}

const char *argus_get_default_bindings_resource_id(void) {
    return argus::get_default_bindings_resource_id().c_str();
}

void argus_set_default_bindings_resource_id(const char *resource_id) {
    argus::set_default_bindings_resource_id(resource_id);
}

bool argus_get_save_user_bindings(void) {
    return argus::get_save_user_bindings();
}

void argus_set_save_user_bindings(bool save) {
    argus::set_save_user_bindings(save);
}

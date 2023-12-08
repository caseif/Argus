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

#include "argus/core_cabi/downstream_config.h"

#include "argus/core/downstream_config.hpp"

static TriState _to_tristate(std::optional<bool> opt) {
    return opt.has_value()
            ? opt.value()
                    ? TriState::TRISTATE_TRUE
                    : TriState::TRISTATE_FALSE
            : TriState::TRISTATE_UNDEF;
}

static const char *_to_c_str(const std::optional<std::string> &opt) {
    return opt.has_value() ? opt.value().c_str() : nullptr;
}

static std::optional<std::string> _from_c_str(const char *c_str) {
    return c_str != nullptr ? std::make_optional(c_str) : std::nullopt;
}

const char *get_main_script(void) {
    auto params = argus::get_scripting_parameters();
    return params.has_value() ? _to_c_str(params->main) : nullptr;
}

void set_main_script(const char *script_uid) {
    auto params = argus::get_scripting_parameters().value_or(argus::ScriptingParameters {});
    params.main = _from_c_str(script_uid);
    argus::set_scripting_parameters(params);
}

const char *get_initial_window_id(void) {
    auto params = argus::get_initial_window_parameters();
    return params.has_value() ? _to_c_str(params->id) : nullptr;
}

void set_initial_window_id(const char *id) {
    auto params = argus::get_initial_window_parameters().value_or(argus::InitialWindowParameters {});
    params.id = _from_c_str(id);
    argus::set_initial_window_parameters(params);
}

const char *get_initial_window_title(void) {
    auto params = argus::get_initial_window_parameters();
    return params.has_value() ? _to_c_str(params->title) : nullptr;
}

void set_initial_window_title(const char *title) {
    auto params = argus::get_initial_window_parameters().value_or(argus::InitialWindowParameters {});
    params.title = _from_c_str(title);
    argus::set_initial_window_parameters(params);
}

const char *get_initial_window_mode(void) {
    auto params = argus::get_initial_window_parameters();
    return params.has_value() ? _to_c_str(params->mode) : nullptr;
}

void set_initial_window_mode(const char *mode) {
    auto params = argus::get_initial_window_parameters().value_or(argus::InitialWindowParameters {});
    params.mode = _from_c_str(mode);
    argus::set_initial_window_parameters(params);
}

TriState get_initial_window_vsync(void) {
    auto params = argus::get_initial_window_parameters();
    return params.has_value() ? _to_tristate(params->vsync) : TriState::TRISTATE_UNDEF;
}

void set_initial_window_vsync(bool vsync) {
    auto params = argus::get_initial_window_parameters().value_or(argus::InitialWindowParameters {});
    params.vsync = vsync;
    argus::set_initial_window_parameters(params);
}

TriState get_initial_window_mouse_visible(void) {
    auto params = argus::get_initial_window_parameters();
    return params.has_value() ? _to_tristate(params->mouse_visible) : TriState::TRISTATE_UNDEF;
}

void set_initial_window_mouse_visible(bool visible) {
    auto params = argus::get_initial_window_parameters().value_or(argus::InitialWindowParameters {});
    params.mouse_visible = visible;
    argus::set_initial_window_parameters(params);
}

TriState get_initial_window_mouse_captured(void) {
    auto params = argus::get_initial_window_parameters();
    return params.has_value() ? _to_tristate(params->mouse_captured) : TriState::TRISTATE_UNDEF;
}

void set_initial_window_mouse_captured(bool captured) {
    auto params = argus::get_initial_window_parameters().value_or(argus::InitialWindowParameters {});
    params.mouse_captured = captured;
    argus::set_initial_window_parameters(params);
}

TriState get_initial_window_mouse_raw_input(void) {
    auto params = argus::get_initial_window_parameters();
    return params.has_value() ? _to_tristate(params->mouse_raw_input) : TriState::TRISTATE_UNDEF;
}

void set_initial_window_mouse_raw_input(bool raw_input) {
    auto params = argus::get_initial_window_parameters().value_or(argus::InitialWindowParameters {});
    params.mouse_raw_input = raw_input;
    argus::set_initial_window_parameters(params);
}

//TODO: get_initial_window_position

//TODO: set_initial_window_position

//TODO: get_initial_window_dimensions

//TODO: set_initial_window_dimensions

const char *get_default_bindings_resource_id(void) {
    return argus::get_default_bindings_resource_id().c_str();
}

void set_default_bindings_resource_id(const char *resource_id) {
    argus::set_default_bindings_resource_id(resource_id);
}

bool get_save_user_bindings(void) {
    return argus::get_save_user_bindings();
}

void set_save_user_bindings(bool save) {
    argus::set_save_user_bindings(save);
}

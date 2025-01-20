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

use std::ptr::null;

use lowlevel_rustabi::argus::lowlevel::Vector2i;
use lowlevel_rustabi::argus::lowlevel::Vector2u;
use lowlevel_rustabi::util::*;

use crate::core_cabi::*;

pub struct ScriptingParameters {
    main: Option<String>,
}

pub struct InitialWindowParameters {
    pub id: Option<String>,
    pub title: Option<String>,
    pub mode: Option<String>,
    pub vsync: Option<bool>,
    pub mouse_visible: Option<bool>,
    pub mouse_captured: Option<bool>,
    pub mouse_raw_input: Option<bool>,
    pub position: Option<Vector2i>,
    pub dimensions: Option<Vector2u>,
}

pub fn get_scripting_parameters() -> ScriptingParameters {
    unsafe {
        let compat_params = argus_get_scripting_parameters();
        return ScriptingParameters {
            main: match compat_params.has_main {
                true => Some(cstr_to_string(compat_params.main)),
                false => None,
            },
        };
    }
}

pub fn set_scripting_parameters(params: &ScriptingParameters) {
    unsafe {
        let main_c = params.main.as_ref().map(|s| string_to_cstring(&s));

        argus_set_scripting_parameters(&argus_scripting_parameters_t {
            has_main: params.main.is_some(),
            main: main_c.map(|s| s.as_ptr()).unwrap_or(null()),
        });
    }
}

pub fn get_initial_window_parameters() -> InitialWindowParameters {
    unsafe {
        let compat_params = argus_get_initial_window_parameters();
        return InitialWindowParameters {
            id: match compat_params.has_id {
                true => Some(cstr_to_string(compat_params.id)),
                false => None,
            },
            title: match compat_params.has_title {
                true => Some(cstr_to_string(compat_params.title)),
                false => None,
            },
            mode: match compat_params.has_mode {
                true => Some(cstr_to_string(compat_params.mode)),
                false => None,
            },
            vsync: match compat_params.has_vsync {
                true => Some(compat_params.vsync),
                false => None,
            },
            mouse_visible: match compat_params.has_mouse_visible {
                true => Some(compat_params.mouse_visible),
                false => None,
            },
            mouse_captured: match compat_params.has_mouse_captured {
                true => Some(compat_params.mouse_captured),
                false => None,
            },
            mouse_raw_input: match compat_params.has_mouse_raw_input {
                true => Some(compat_params.mouse_raw_input),
                false => None,
            },
            position: match compat_params.has_position {
                true => Some(compat_params.position.into()),
                false => None,
            },
            dimensions: match compat_params.has_dimensions {
                true => Some(compat_params.dimensions.into()),
                false => None,
            },
        };
    }
}

pub fn set_initial_window_parameters(params: &InitialWindowParameters) {
    unsafe {
        let id_c = params.id.as_ref().map(|s| string_to_cstring(&s));
        let title_c = params.title.as_ref().map(|s| string_to_cstring(&s));
        let mode_c = params.title.as_ref().map(|s| string_to_cstring(&s));

        argus_set_initial_window_parameters(&argus_initial_window_parameters_t {
            has_id: params.id.is_some(),
            id: id_c.map(|s| s.as_ptr()).unwrap_or(null()),
            has_title: params.title.is_some(),
            title: title_c.map(|s| s.as_ptr()).unwrap_or(null()),
            has_mode: params.mode.is_some(),
            mode: mode_c.map(|s| s.as_ptr()).unwrap_or(null()),
            has_vsync: params.vsync.is_some(),
            vsync: params.vsync.unwrap_or_default(),
            has_mouse_visible: params.mouse_visible.is_some(),
            mouse_visible: params.mouse_visible.unwrap_or_default(),
            has_mouse_captured: params.mouse_captured.is_some(),
            mouse_captured: params.mouse_captured.unwrap_or_default(),
            has_mouse_raw_input: params.mouse_raw_input.is_some(),
            mouse_raw_input: params.mouse_raw_input.unwrap_or_default(),
            has_position: params.position.is_some(),
            position: params.position.unwrap_or(std::mem::zeroed()).into(),
            has_dimensions: params.dimensions.is_some(),
            dimensions: params.dimensions.unwrap_or(std::mem::zeroed()).into(),
        });
    }
}

pub fn get_default_bindings_resource_id() -> String {
    unsafe {
        return cstr_to_string(argus_get_default_bindings_resource_id());
    }
}

pub fn set_default_bindings_resource_id(resource_id: &str) {
    unsafe {
        let resource_id_c = str_to_cstring(resource_id);
        argus_set_default_bindings_resource_id(resource_id_c.as_ptr());
    }
}

pub fn get_save_user_bindings() -> bool {
    unsafe {
        return argus_get_save_user_bindings();
    }
}

pub fn set_save_user_bindings(save: bool) {
    unsafe {
        argus_set_save_user_bindings(save);
    }
}

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

use std::convert::TryFrom;
use std::ffi::{CString, c_char};
use std::ptr::null_mut;

use crate::argus::core::screen_space::ScreenSpaceScaleMode;
use crate::bindings;
use crate::util::*;

pub fn set_target_tickrate(target_tickrate: u32) {
    unsafe {
        bindings::set_target_tickrate(target_tickrate);
    }
}

pub fn set_target_framerate(target_framerate: u32) {
    unsafe {
        bindings::set_target_framerate(target_framerate);
    }
}

pub fn set_load_modules(module_names: Vec<String>) {
    unsafe {
        let (names, count) = string_vec_to_cstr_arr(module_names).into();
        bindings::set_load_modules(names, count);
    }
}

pub fn add_load_module(module_name: &str) {
    unsafe {
        bindings::add_load_module(str_to_cstring(module_name).as_ptr());
    }
}

pub fn get_preferred_render_backends() -> Vec<String> {
    unsafe {
        let mut count: usize = 0;
        bindings::get_preferred_render_backends(&mut count, null_mut());

        let mut names = Vec::<*const c_char>::new();
        bindings::get_preferred_render_backends(null_mut(), names.as_mut_ptr());

        return names.iter().map(|s| cstr_to_string(*s)).collect();
    }
}

pub fn set_render_backends(names: Vec<String>) {
    unsafe {
        let (names, count) = string_vec_to_cstr_arr(names).into();
        bindings::set_render_backends(names, count);
    }
}

pub fn add_render_backend(name: &str) {
    unsafe {
        bindings::add_render_backend(str_to_cstring(name).as_ptr());
    }
}

pub fn set_render_backend(name: &str) {
    unsafe {
        bindings::set_render_backend(str_to_cstring(name).as_ptr());
    }
}

pub fn get_screen_space_scale_mode() -> ScreenSpaceScaleMode {
    unsafe {
        return ScreenSpaceScaleMode::try_from(bindings::get_screen_space_scale_mode()).unwrap();
    }
}

pub fn set_screen_space_scale_mode(mode: ScreenSpaceScaleMode) {
    unsafe {
        bindings::set_screen_space_scale_mode(mode as bindings::ScreenSpaceScaleMode);
    }
}

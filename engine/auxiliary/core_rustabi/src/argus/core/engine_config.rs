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

use lowlevel_rustabi::util::*;

use crate::argus::core::screen_space::ScreenSpaceScaleMode;
use crate::core_cabi;

pub fn set_target_tickrate(target_tickrate: u32) {
    unsafe {
        core_cabi::set_target_tickrate(target_tickrate);
    }
}

pub fn set_target_framerate(target_framerate: u32) {
    unsafe {
        core_cabi::set_target_framerate(target_framerate);
    }
}

pub fn set_load_modules(module_names: Vec<String>) {
    unsafe {
        let (names, count) = string_vec_to_cstr_arr(module_names).into();
        core_cabi::set_load_modules(names, count);
    }
}

pub fn add_load_module(module_name: &str) {
    unsafe {
        core_cabi::add_load_module(str_to_cstring(module_name).as_ptr());
    }
}

pub fn get_preferred_render_backends() -> Vec<String> {
    unsafe {
        let mut count: usize = 0;
        core_cabi::get_preferred_render_backends(&mut count, null_mut());

        let mut names = Vec::<*const c_char>::new();
        core_cabi::get_preferred_render_backends(null_mut(), names.as_mut_ptr());

        return names.iter().map(|s| cstr_to_string(*s)).collect();
    }
}

pub fn set_render_backends(names: Vec<String>) {
    unsafe {
        let (names, count) = string_vec_to_cstr_arr(names).into();
        core_cabi::set_render_backends(names, count);
    }
}

pub fn add_render_backend(name: &str) {
    unsafe {
        core_cabi::add_render_backend(str_to_cstring(name).as_ptr());
    }
}

pub fn set_render_backend(name: &str) {
    unsafe {
        core_cabi::set_render_backend(str_to_cstring(name).as_ptr());
    }
}

pub fn get_screen_space_scale_mode() -> ScreenSpaceScaleMode {
    unsafe {
        return ScreenSpaceScaleMode::try_from(core_cabi::get_screen_space_scale_mode()).unwrap();
    }
}

pub fn set_screen_space_scale_mode(mode: ScreenSpaceScaleMode) {
    unsafe {
        core_cabi::set_screen_space_scale_mode(mode as core_cabi::ScreenSpaceScaleMode);
    }
}

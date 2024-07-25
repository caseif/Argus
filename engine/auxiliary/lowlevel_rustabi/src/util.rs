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

use std::ffi::{c_char, CStr, CString};

pub struct CStringArray {
    strs: Vec<CString>,
    ptrs: Vec<*const c_char>,
}

impl CStringArray {
    pub fn new(strs: Vec<CString>) -> Self {
        let mut arr = Self { strs, ptrs: vec![] };
        arr.ptrs = arr
            .strs
            .iter()
            .map(|s| s.as_ptr())
            .collect::<Vec<_>>();
        arr
    }

    pub fn as_ptr(&self) -> *const *const c_char {
        self.ptrs.as_ptr()
    }
}

pub fn string_to_cstring(s: &String) -> CString {
    CString::new(s.to_string()).unwrap()
}

pub fn str_to_cstring(s: &str) -> CString {
    string_to_cstring(&s.to_string())
}

pub unsafe fn cstr_to_string(s: *const c_char) -> String {
    CStr::from_ptr(s).to_str().unwrap().to_string()
}

pub unsafe fn cstr_to_str<'a>(s: *const c_char) -> &'a str {
    CStr::from_ptr(s)
        .to_str()
        .expect("Failed to convert C string to Rust string")
}

pub unsafe fn str_vec_to_cstr_arr(v: &Vec<&str>) -> CStringArray {
    CStringArray::new(v.into_iter().map(|s| str_to_cstring(*s)).collect())
}

pub unsafe fn string_vec_to_cstr_arr(v: &Vec<String>) -> CStringArray {
    CStringArray::new(v.iter().map(string_to_cstring).collect())
}

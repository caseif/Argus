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

use std::ffi::{c_char, CStr, CString, OsString};

pub struct CStringArray {
    vec: Vec<CString>,
}

impl Into<*const *const c_char> for CStringArray {
    fn into(self) -> *const *const c_char {
        return self
            .vec
            .iter()
            .map(|s| s.as_ptr())
            .collect::<Vec<_>>()
            .as_ptr();
    }
}

impl Into<(*const *const c_char, usize)> for CStringArray {
    fn into(self) -> (*const *const c_char, usize) {
        let len = self.vec.len();
        return (self.into(), len);
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
    String::from_utf16()
    CStr::from_ptr(s)
        .to_str()
        .expect("Failed to convert C string to Rust string")
}

pub unsafe fn string_vec_to_cstr_arr(v: Vec<String>) -> CStringArray {
    CStringArray {
        vec: v.iter().map(string_to_cstring).collect(),
    }
}

#[cfg(windows)]
pub unsafe fn os_string_to_string<T>(s: *const c_char) -> String {
    let len = (0..).take_while(|&i| *s.offset(i) != 0).count();
    let slice = std::slice::from_raw_parts(s, len);
    OsString::from_wide(slice)
}

#[cfg(not(windows))]
pub unsafe fn os_string_to_string<T>(s: *const c_char) -> String {
    cstr_to_string(s)
}

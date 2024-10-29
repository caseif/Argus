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
use std::ffi;
use std::ffi::CStr;

use crate::lowlevel_cabi::*;

use crate::lowlevel_cabi;
pub use crate::lowlevel_cabi::StringArray;
pub use crate::lowlevel_cabi::StringArrayConst;

pub trait FfiWrapper {
    fn of(ptr: *mut ffi::c_void) -> Self;
}

pub trait FfiRepr<C>: From<C> + Into<C> {
}

pub unsafe fn string_array_to_vec(sa: StringArray) -> Vec<String> {
    unsafe {
        let count = string_array_get_count(sa);
        let mut vec = Vec::<String>::with_capacity(count);

        for i in 0..count {
            let s = string_array_get_element(sa, i);
            vec[i] = CStr::from_ptr(s).to_str().unwrap().to_string();
        }

        vec
    }
}

pub fn free_string_array(sa: StringArray) {
    unsafe {
        bindings::string_array_free(sa);
    }
}

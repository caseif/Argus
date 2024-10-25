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

use std::ffi::CStr;
use argus_scripting_bind::ReflectiveArgumentsError;
use crate::scripting_cabi::*;

#[derive(Clone, Debug)]
pub struct BindingError {
    pub bound_name: String,
    pub msg: String,
}

impl From<argus_binding_error_t> for BindingError {
    fn from(err_ffi: argus_binding_error_t) -> Self {
        unsafe {
            let error = Self {
                bound_name: CStr::from_ptr(argus_binding_error_get_bound_name(err_ffi))
                    .to_string_lossy().to_string(),
                msg: CStr::from_ptr(argus_binding_error_get_msg(err_ffi))
                    .to_string_lossy().to_string(),
            };

            argus_binding_error_free(err_ffi);

            error
        }
    }
}

pub fn translate_refl_args_err(err_ffi: argus_reflective_args_error_t) -> ReflectiveArgumentsError {
    unsafe {
        let error = ReflectiveArgumentsError {
            reason: CStr::from_ptr(argus_reflective_args_error_get_reason(err_ffi))
                .to_string_lossy().to_string(),
        };

        argus_reflective_args_error_free(err_ffi);

        error
    }
}

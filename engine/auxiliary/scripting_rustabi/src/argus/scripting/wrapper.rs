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
use crate::argus::scripting::{FfiObjectType, FfiObjectWrapper};
use crate::scripting_cabi::*;

fn unwrap_res(res: ArgusObjectWrapperOrReflectiveArgsError) -> Result<FfiObjectWrapper, String> {
    if res.is_err {
        let msg = unsafe {
            CStr::from_ptr(argus_reflective_args_error_get_reason(res.err))
                .to_string_lossy().to_string()
        };
        unsafe { argus_object_wrapper_or_refl_args_err_delete(res) };
        Err(msg)
    } else {
        Ok(FfiObjectWrapper::of(res.val))
    }
}

pub fn create_object_wrapper(ty: FfiObjectType, ptr: *mut (), size: usize)
    -> Result<FfiObjectWrapper, String> {
    unwrap_res(unsafe { argus_create_object_wrapper(ty.handle, ptr.cast(), size) })
}

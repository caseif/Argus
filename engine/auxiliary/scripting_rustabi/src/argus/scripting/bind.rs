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

use std::ffi::CString;
use std::{ffi, ptr};
use std::any::Any;
use crate::argus::scripting::{BindingError, ObjectType};
use crate::scripting_cabi::*;

pub use argus_scripting_bind;
pub use argus_scripting_bind::script_bind;

fn unwrap_bind_result(result: ArgusMaybeBindingError) -> Result<(), BindingError> {
    if result.is_err {
        Err(result.error.into())
    } else {
        Ok(())
    }
}

pub fn bind_type(
    name: &str,
    size: usize,
    type_id: &str,
    is_refable: bool,
    copy_ctor: ArgusCopyCtorProxy,
    move_ctor: ArgusMoveCtorProxy,
    dtor: ArgusDtorProxy,
) -> Result<(), BindingError> {
    let name_c = CString::new(name).unwrap();
    let type_id_c = CString::new(type_id).unwrap();
    let res = unsafe {
        argus_bind_type(
            name_c.as_ptr(),
            size,
            type_id_c.as_ptr(),
            is_refable,
            copy_ctor,
            move_ctor,
            dtor,
        )
    };

    unwrap_bind_result(res)
}

pub fn bind_enum_type(
    name: &str,
    width: usize,
    type_id: &str,
) -> Result<(), BindingError> {
    let name_c = CString::new(name).unwrap();
    let type_id_c = CString::new(type_id).unwrap();
    let res = unsafe { argus_bind_enum(name_c.as_ptr(), width, type_id_c.as_ptr()) };

    unwrap_bind_result(res)
}

pub fn bind_enum_value(
    enum_type_id: &str,
    name: &str,
    value: i64,
) -> Result<(), BindingError> {
    let name_c = CString::new(name).unwrap();
    let enum_type_id_c = CString::new(enum_type_id).unwrap();
    let res = unsafe {
        argus_bind_enum_value(
            enum_type_id_c.as_ptr(),
            name_c.as_ptr(),
            value,
        )
    };

    unwrap_bind_result(res)
}

pub fn bind_global_function(
    name: &str,
    params: Vec<ObjectType>,
    ret_type: ObjectType,
    fn_proxy: ArgusProxiedNativeFunction,
    extra: *mut ffi::c_void,
) -> Result<(), BindingError> {
    let name_c = CString::new(name).unwrap();
    let params_ffi: Vec<argus_object_type_const_t> =
        params.iter().map(|p| p.handle.cast_const()).collect();
    let res = unsafe {
        argus_bind_global_function(
            name_c.as_ptr(),
            params.len(),
            params_ffi.as_ptr(),
            ret_type.handle,
            fn_proxy,
            extra,
        )
    };

    unwrap_bind_result(res)
}

pub fn bind_member_static_function(
    type_id: &str,
    name: &str,
    params_count: usize,
    params: Vec<ObjectType>,
    ret_type: ObjectType,
    proxied_fn: ArgusProxiedNativeFunction,
) -> Result<(), BindingError> {
    Ok(()) //TODO
}

pub fn bind_member_instance_function(
    type_id: &str,
    name: &str,
    is_const: bool,
    params: Vec<ObjectType>,
    ret_type: ObjectType,
    proxied_fn: ArgusProxiedNativeFunction,
) -> Result<(), BindingError> {
    Ok(()) //TODO
}

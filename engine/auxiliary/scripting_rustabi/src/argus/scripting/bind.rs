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

use argus_scripting_bind::{ObjectType, ProxiedNativeFunction};
use std::ffi::CString;
use std::{ffi, mem, ptr};
use std::any::TypeId;
use std::ptr::slice_from_raw_parts;
use crate::argus::scripting::{call_proxied_fn, BindingError, FfiObjectType, FfiObjectWrapper};
use crate::scripting_cabi::*;

pub use argus_scripting_bind;
pub use argus_scripting_bind::script_bind;

type FieldAccessor = dyn Fn(*const (), FfiObjectType) -> FfiObjectWrapper;
type FieldMutator = dyn Fn(*mut (), FfiObjectWrapper);

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

fn create_object_type(_ty: &ObjectType) -> FfiObjectType {
    FfiObjectType::of(ptr::null_mut()) //TODO
}

pub fn bind_member_field(
    containing_type: TypeId,
    name: &str,
    ty: &FfiObjectType,
    accessor: Box<FieldAccessor>,
    mutator: Box<FieldMutator>,
) -> Result<(), BindingError> {
    let type_id_str = format!("{:?}", containing_type);

    let type_id_c = CString::new(type_id_str).unwrap();
    let name_c = CString::new(name).unwrap();

    #[allow(clippy::missing_transmute_annotations)]
    let res = unsafe {
        unsafe extern "C" fn access(
            inst: *const ffi::c_void,
            ty: argus_object_type_const_t,
            state: *const ffi::c_void,
        ) -> argus_object_wrapper_t {
            let f: &Box<FieldAccessor> = &*state.cast();
            f.as_ref()(inst.cast(), FfiObjectType::of(ty.cast_mut())).handle
        }

        unsafe extern "C" fn mutate(
            inst: *mut ffi::c_void,
            obj2: argus_object_wrapper_t,
            state: *const ffi::c_void,
        ) {
            let f: &Box<FieldMutator> = &*state.cast();
            f.as_ref()(inst.cast(), FfiObjectWrapper::of(obj2));
        }

        argus_bind_member_field(
            type_id_c.as_ptr(),
            name_c.as_ptr(),
            ty.handle,
            Some(access),
            Box::into_raw(Box::new(accessor)).cast(),
            Some(mutate),
            Box::into_raw(Box::new(mutator)).cast(),
        )
    };

    unwrap_bind_result(res)
}

pub fn bind_global_function(
    name: &str,
    params: Vec<FfiObjectType>,
    ret_type: FfiObjectType,
    fn_proxy: &ProxiedNativeFunction,
) -> Result<(), BindingError> {
    let name_c = CString::new(name).unwrap();
    let params_ffi: Vec<argus_object_type_const_t> =
        params.iter().map(|p| p.handle.cast()).collect();
    let res = unsafe {
        unsafe extern "C" fn delegate(
            params_count: usize,
            params: *const argus_object_wrapper_t,
            extra: *const ffi::c_void
        ) -> ArgusObjectWrapperOrReflectiveArgsError {
            let params_slice = &*slice_from_raw_parts(params, params_count);

            let f: &Box<ProxiedNativeFunction> = mem::transmute(extra);
            call_proxied_fn(f, params_slice)
        }

        argus_bind_global_function(
            name_c.as_ptr(),
            params.len(),
            params_ffi.as_ptr(),
            ret_type.handle,
            Some(delegate),
            mem::transmute(Box::into_raw(Box::new(fn_proxy))),
        )
    };

    unwrap_bind_result(res)
}

pub fn bind_member_static_function(
    type_id: &str,
    name: &str,
    params_count: usize,
    params: Vec<FfiObjectType>,
    ret_type: FfiObjectType,
    proxied_fn: Box<ProxiedNativeFunction>,
) -> Result<(), BindingError> {
    Ok(()) //TODO
}

pub fn bind_member_instance_function(
    type_id: &str,
    name: &str,
    is_const: bool,
    params: Vec<FfiObjectType>,
    ret_type: FfiObjectType,
    proxied_fn: Box<ProxiedNativeFunction>,
) -> Result<(), BindingError> {
    Ok(()) //TODO
}

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

use argus_scripting_types::{FunctionType, ObjectType, ProxiedNativeFunction, ReflectiveArgumentsError, WrappedObject};
use std::ffi::CString;
use std::{ffi, mem, ptr};
use std::any::TypeId;
use std::ptr::slice_from_raw_parts;
use crate::argus::scripting::{call_proxied_fn, to_ffi_obj_wrapper, BindingError, FfiObjectType, FfiObjectWrapper, ScriptManager};
use crate::scripting_cabi::*;

type FieldAccessor = dyn Fn(*const (), FfiObjectType) -> FfiObjectWrapper;
type FieldMutator = dyn Fn(*mut (), FfiObjectWrapper);

/*fn unwrap_create_def_result(result: ArgusObjectWrapperOrReflectiveArgsError)
    -> Result<WrappedObject, ReflectiveArgumentsError> {
    if result.is_err {
        let err_rs = result.err.into();
        unsafe { argus_reflective_args_error_free(result.err) };
        Err(err_rs)
    } else {
        let obj_rs = from_ffi_obj_type(FfiObjectType::of(result.val));
        unsafe { argus_object_wrapper_delete(result.val) };
        Ok(obj_rs)
    }
}*/

pub struct BoundTypeDef {
    handle: argus_bound_type_def_t,
}

impl BoundTypeDef {
    pub(crate) fn of(handle: argus_bound_type_def_t) -> Self {
        Self { handle }
    }

    pub(crate) fn get_handle(&self) -> argus_bound_type_def_t {
        self.handle
    }
}

impl Drop for BoundTypeDef {
    fn drop(&mut self) {
        unsafe { argus_bound_type_def_delete(self.handle) };
    }
}

pub struct BoundEnumDef {
    handle: argus_bound_enum_def_t,
}

impl BoundEnumDef {
    pub(crate) fn of(handle: argus_bound_enum_def_t) -> Self {
        Self { handle }
    }

    pub(crate) fn get_handle(&self) -> argus_bound_enum_def_t {
        self.handle
    }
}

impl Drop for BoundEnumDef {
    fn drop(&mut self) {
        unsafe { argus_bound_enum_def_delete(self.handle) };
    }
}

pub struct BoundFunctionDef {
    handle: argus_bound_function_def_t,
}

impl BoundFunctionDef {
    pub(crate) fn of(handle: argus_bound_function_def_t) -> Self {
        Self { handle }
    }
    
    pub(crate) fn get_handle(&self) -> argus_bound_function_def_t {
        self.handle
    }
}

impl Drop for BoundFunctionDef {
    fn drop(&mut self) {
        unsafe { argus_bound_function_def_delete(self.handle) };
    }
}

pub fn create_type_def(
    name: &str,
    size: usize,
    type_id: &str,
    is_refable: bool,
    copy_ctor: ArgusCopyCtorProxy,
    move_ctor: ArgusMoveCtorProxy,
    dtor: ArgusDtorProxy,
) -> Result<BoundTypeDef, BindingError> {
    let name_c = CString::new(name).unwrap();
    let type_id_c = CString::new(type_id).unwrap();
    let res = unsafe {
        argus_create_type_def(
            name_c.as_ptr(),
            size,
            type_id_c.as_ptr(),
            is_refable,
            copy_ctor,
            move_ctor,
            dtor,
        )
    };

    if res.is_null() {
        panic!("Failed to create bound type definition");
    }

    Ok(BoundTypeDef::of(res))
}

pub fn create_enum_def(
    name: &str,
    width: usize,
    type_id: &str,
) -> Result<BoundEnumDef, BindingError> {
    let name_c = CString::new(name).unwrap();
    let type_id_c = CString::new(type_id).unwrap();
    let res = unsafe { argus_create_enum_def(name_c.as_ptr(), width, type_id_c.as_ptr()) };

    Ok(BoundEnumDef::of(res))
}

pub fn add_enum_value(
    enum_def: &BoundEnumDef,
    name: &str,
    value: i64,
) -> Result<(), BindingError> {
    let name_c = CString::new(name).unwrap();
    unsafe {
        argus_add_enum_value(
            enum_def.get_handle(),
            name_c.as_ptr(),
            value,
        ).into()
    }
}

fn create_object_type(_ty: &ObjectType) -> FfiObjectType {
    FfiObjectType::of(ptr::null_mut()) //TODO
}

pub fn add_member_field(
    type_def: &BoundTypeDef,
    name: &str,
    ty: &FfiObjectType,
    accessor: Box<FieldAccessor>,
    mutator: Box<FieldMutator>,
) -> Result<(), BindingError> {
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

        argus_add_member_field(
            type_def.get_handle(),
            name_c.as_ptr(),
            ty.handle,
            Some(access),
            Box::into_raw(Box::new(accessor)).cast(),
            Some(mutate),
            Box::into_raw(Box::new(mutator)).cast(),
        )
    };

    res.into()
}

fn add_function(
    type_def: Option<&BoundTypeDef>,
    ty: FunctionType,
    name: &str,
    params: &[FfiObjectType],
    ret_type: FfiObjectType,
    fn_proxy: ProxiedNativeFunction,
    is_const: bool,
)
    -> Result<(), BindingError> {
    assert!(type_def.is_some() || ty == FunctionType::Global);
    
    let name_c = CString::new(name).unwrap();
    let params_ffi: Vec<argus_object_type_const_t> =
        params.iter().map(|p| p.handle.cast()).collect();
    unsafe {
        unsafe extern "C" fn delegate(
            params_count: usize,
            params: *const argus_object_wrapper_t,
            extra: *const ffi::c_void
        ) -> ArgusObjectWrapperOrReflectiveArgsError {
            let params_slice = &*slice_from_raw_parts(params, params_count);

            let f: &ProxiedNativeFunction = mem::transmute(extra);
            call_proxied_fn(f, params_slice)
        }

        match ty {
            FunctionType::Global => {
                let fn_def = argus_create_global_function_def(
                    name_c.as_ptr(),
                    false,
                    params.len(),
                    params_ffi.as_ptr(),
                    ret_type.handle,
                    Some(delegate),
                    mem::transmute(Box::into_raw(Box::new(fn_proxy))),
                );
                ScriptManager::instance().bind_global_function(BoundFunctionDef::of(fn_def));
                Ok(())
            }
            FunctionType::MemberStatic => {
                argus_add_member_static_function(
                    type_def.unwrap().get_handle(),
                    name_c.as_ptr(),
                    params.len(),
                    params_ffi.as_ptr(),
                    ret_type.handle,
                    Some(delegate),
                    mem::transmute(Box::into_raw(Box::new(fn_proxy))),
                ).into()
            }
            FunctionType::MemberInstance => {
                argus_add_member_instance_function(
                    type_def.unwrap().get_handle(),
                    name_c.as_ptr(),
                    is_const,
                    params.len(),
                    params_ffi.as_ptr(),
                    ret_type.handle,
                    Some(delegate),
                    mem::transmute(Box::into_raw(Box::new(fn_proxy))),
                ).into()
            }
            FunctionType::Extension => {
                panic!("Binding extension functions is not supported at this time");
            }
        }
    }
}

pub fn add_member_static_function(
    type_def: &BoundTypeDef,
    name: &str,
    params: Vec<FfiObjectType>,
    ret_type: FfiObjectType,
    fn_proxy: ProxiedNativeFunction,
) -> Result<(), BindingError> {
    add_function(
        Some(type_def),
        FunctionType::MemberStatic,
        name,
        params.as_slice(),
        ret_type,
        fn_proxy,
        false,
    )
}

pub fn add_member_instance_function(
    type_def: &BoundTypeDef,
    name: &str,
    is_const: bool,
    params: Vec<FfiObjectType>,
    ret_type: FfiObjectType,
    fn_proxy: ProxiedNativeFunction,
) -> Result<(), BindingError> {
    add_function(
        Some(type_def),
        FunctionType::MemberInstance,
        name,
        &params[1..],
        ret_type,
        fn_proxy,
        is_const,
    )
}

pub fn bind_global_function(
    name: &str,
    params: Vec<FfiObjectType>,
    ret_type: FfiObjectType,
    fn_proxy: ProxiedNativeFunction,
) -> Result<(), BindingError> {
    add_function(
        None,
        FunctionType::Global,
        name,
        params.as_slice(),
        ret_type,
        fn_proxy,
        false,
    )
}

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

use std::{ptr, slice};
use std::ffi::{CStr, CString};
use std::marker::PhantomData;
use num_enum::{IntoPrimitive, UnsafeFromPrimitive};
use argus_scripting_types::{FfiCopyCtor, FfiMoveCtor, ObjectType};
use crate::argus::scripting::{from_ffi_obj_type, to_ffi_obj_type};
use crate::scripting_cabi::*;

#[repr(u32)]
#[derive(Clone, Copy, Debug, IntoPrimitive, UnsafeFromPrimitive)]
pub enum FfiIntegralType {
    Void = INTEGRAL_TYPE_VOID,
    Integer = INTEGRAL_TYPE_INTEGER,
    UInteger = INTEGRAL_TYPE_UINTEGER,
    Float = INTEGRAL_TYPE_FLOAT,
    Boolean = INTEGRAL_TYPE_BOOLEAN,
    String = INTEGRAL_TYPE_STRING,
    Struct = INTEGRAL_TYPE_STRUCT,
    Pointer = INTEGRAL_TYPE_POINTER,
    Enum = INTEGRAL_TYPE_ENUM,
    Callback = INTEGRAL_TYPE_CALLBACK,
    Type = INTEGRAL_TYPE_TYPE,
    Vector = INTEGRAL_TYPE_VECTOR,
    Vectorref = INTEGRAL_TYPE_VECTORREF,
    Result = INTEGRAL_TYPE_RESULT,
}

#[repr(u32)]
#[derive(IntoPrimitive, UnsafeFromPrimitive)]
pub enum FfiFunctionType {
    Global = FUNCTION_TYPE_GLOBAL,
    MemberStatic = FUNCTION_TYPE_MEMBER_STATIC,
    MemberInstance = FUNCTION_TYPE_MEMBER_INSTANCE,
    Extension = FUNCTION_TYPE_EXTENSION,
}

#[repr(u32)]
#[derive(IntoPrimitive, UnsafeFromPrimitive)]
pub enum FfiSymbolType {
    Type = SYMBOL_TYPE_TYPE,
    Field = SYMBOL_TYPE_FIELD,
    Function = SYMBOL_TYPE_FUNCTION,
}

pub struct FfiObjectType<'a> {
    pub handle: argus_object_type_const_t,
    _handle_phantom: PhantomData<&'a ()>,
    must_free: bool,
}

impl<'a> FfiObjectType<'a> {
    pub fn of(handle: argus_object_type_const_t) -> Self {
        Self { handle, _handle_phantom: PhantomData, must_free: false }
    }

    pub fn new(
        ty: FfiIntegralType,
        size: usize,
        is_const: bool,
        is_refable: bool,
        type_id: Option<String>,
        script_callback_type: Option<ScriptCallbackType>,
        primary_type: Option<FfiObjectType<'a>>,
        secondary_type: Option<FfiObjectType<'a>>,
        _copy_ctor: Option<FfiCopyCtor>,
        _move_ctor: Option<FfiMoveCtor>,
    ) -> Self {
        let type_id_c = type_id.map(|id| CString::new(id).unwrap());
        let type_id_c_ptr = type_id_c.as_ref().map(|id| id.as_ptr()).unwrap_or(ptr::null());

        let script_cb_type = script_callback_type.as_ref().map(|t| t.handle).unwrap_or(ptr::null_mut());
        let prim_type = primary_type.as_ref().map(|t| t.handle).unwrap_or(ptr::null_mut());
        let sec_type = secondary_type.as_ref().map(|t| t.handle).unwrap_or(ptr::null_mut());
        let integ_type: ArgusIntegralType = ty.into();
        Self {
            handle: unsafe { argus_object_type_new(
                integ_type,
                size,
                is_const,
                is_refable,
                type_id_c_ptr,
                script_cb_type,
                prim_type,
                sec_type,
            ) },
            _handle_phantom: PhantomData,
            must_free: true,
        }
    }

    pub fn get_handle(&self) -> argus_object_type_const_t {
        self.handle
    }

    pub fn get_type(&self) -> FfiIntegralType {
        unsafe { FfiIntegralType::unchecked_transmute_from(argus_object_type_get_type(self.handle)) }
    }

    pub fn get_size(&self) -> usize {
        unsafe { argus_object_type_get_size(self.handle) }
    }

    pub fn get_is_const(&self) -> bool {
        unsafe { argus_object_type_get_is_const(self.handle) }
    }

    pub fn get_is_refable(&self) -> bool {
        unsafe { argus_object_type_get_is_refable(self.handle) }
    }

    pub fn get_type_id(&self) -> String {
        unsafe {
            CStr::from_ptr(argus_object_type_get_type_id(self.handle)).to_string_lossy().to_string()
        }
    }

    pub fn get_type_name(&self) -> Option<String> {
        unsafe {
            let name_c = argus_object_type_get_type_name(self.handle);
            if name_c.is_null() {
                return None;
            }
            Some(CStr::from_ptr(name_c).to_string_lossy().to_string())
        }
    }

    pub fn get_callback_type(&self) -> Option<ScriptCallbackType> {
        unsafe {
            argus_object_type_get_callback_type(self.handle).cast_mut().as_mut()
                .map(|p| ScriptCallbackType::of(ptr::from_mut(p)))
        }
    }

    pub fn get_primary_type(&self) -> Option<FfiObjectType> {
        unsafe {
            argus_object_type_get_primary_type(self.handle).cast_mut().as_mut()
                .map(|p| FfiObjectType::of(ptr::from_mut(p)))
        }
    }

    pub fn get_secondary_type(&self) -> Option<FfiObjectType> {
        unsafe {
            argus_object_type_get_secondary_type(self.handle).cast_mut().as_mut()
                .map(|p| FfiObjectType::of(ptr::from_mut(p)))
        }
    }
}

impl<'a> Drop for FfiObjectType<'a> {
    fn drop(&mut self) {
        if self.must_free {
            unsafe { argus_object_type_delete(self.handle.cast_mut()); }
        }
    }
}

pub struct ScriptCallbackType {
    handle: argus_script_callback_type_t,
    must_free: bool,
}

impl Drop for ScriptCallbackType {
    fn drop(&mut self) {
        if self.must_free {
            unsafe { argus_script_callback_type_delete(self.handle); }
        }
    }
}

impl ScriptCallbackType {
    pub fn of(handle: argus_script_callback_type_t) -> Self {
        Self { handle, must_free: false }
    }
    
    pub fn new(param_types: &[FfiObjectType], ret_type: &FfiObjectType) -> Self {
        let param_type_ptrs = param_types.iter().map(|ty| ty.handle).collect::<Vec<_>>();
        Self {
            handle: unsafe {
                argus_script_callback_type_new(
                    param_types.len(),
                    param_type_ptrs.as_ptr(),
                    ret_type.get_handle(),
                )
            },
            must_free: true,
        }
    }

    pub fn from(param_types: &[ObjectType], return_type: &ObjectType) -> Self {
        let ffi_param_types = param_types.iter()
            .map(|ty| {
                to_ffi_obj_type(ty)
            })
            .collect::<Vec<_>>();

        let ffi_ret_type = to_ffi_obj_type(return_type);
        Self::new(&ffi_param_types, &ffi_ret_type)
    }

    pub fn get_param_types(&self) -> Vec<ObjectType> {
        unsafe {
            let count = argus_script_callback_type_get_param_count(self.handle.cast());
            let mut ty_ptrs = Vec::with_capacity(count);
            ty_ptrs.resize(count, ptr::null_mut());
            argus_script_callback_type_get_params(self.handle.cast(), ty_ptrs.as_mut_ptr(), count);
            ty_ptrs.iter().map(|ty| from_ffi_obj_type(&FfiObjectType::of(*ty))).collect()
        }
    }

    pub fn get_return_type(&self) -> ObjectType {
        unsafe {
            from_ffi_obj_type(&FfiObjectType::of(
                argus_script_callback_type_get_return_type(self.handle.cast())
            ))
        }
    }
}

pub struct FfiObjectWrapper {
    pub(crate) handle: argus_object_wrapper_t,
    must_free: bool,
}

impl FfiObjectWrapper {
    pub fn of(handle: argus_object_wrapper_t) -> Self {
        Self { handle, must_free: false }
    }

    pub fn new(ty: FfiObjectType, size: usize) -> Self {
        Self {
            handle: unsafe { argus_object_wrapper_new(ty.handle, size) },
            must_free: true,
        }
    }

    pub fn get_value(&self) -> *const () {
        unsafe { argus_object_wrapper_get_value(self.handle).cast() }
    }

    pub fn get_value_mut(&mut self) -> *mut () {
        unsafe { argus_object_wrapper_get_value_mut(self.handle).cast() }
    }

    pub fn get_buffer<'a>(&'a self) -> &'a [u8] {
        unsafe {
            slice::from_raw_parts(
                argus_object_wrapper_get_value(self.handle).cast(),
                argus_object_wrapper_get_buffer_size(self.handle),
            )
        }
    }
}

impl Drop for FfiObjectWrapper {
    fn drop(&mut self) {
        if self.must_free {
            unsafe { argus_object_wrapper_delete(self.handle); }
        }
    }
}

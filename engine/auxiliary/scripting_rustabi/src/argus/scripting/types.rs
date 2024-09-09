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

use std::{ffi, ptr, slice};
use std::ffi::{CStr, CString};
use std::marker::PhantomData;
use num_enum::{IntoPrimitive, UnsafeFromPrimitive};
use crate::scripting_cabi::*;

#[repr(u32)]
#[derive(IntoPrimitive, UnsafeFromPrimitive)]
pub enum IntegralType {
    Void = INTEGRAL_TYPE_VOID,
    Integer = INTEGRAL_TYPE_INTEGER,
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
pub enum FunctionType {
    Global = FUNCTION_TYPE_GLOBAL,
    MemberStatic = FUNCTION_TYPE_MEMBER_STATIC,
    MemberInstance = FUNCTION_TYPE_MEMBER_INSTANCE,
    Extension = FUNCTION_TYPE_EXTENSION,
}

#[repr(u32)]
#[derive(IntoPrimitive, UnsafeFromPrimitive)]
pub enum SymbolType {
    Type = SYMBOL_TYPE_TYPE,
    Field = SYMBOL_TYPE_FIELD,
    Function = SYMBOL_TYPE_FUNCTION,
}

pub struct ObjectType {
    pub(crate) handle: argus_object_type_t,
    must_delete: bool,
}

impl ObjectType {
    pub fn of(handle: argus_object_type_t) -> Self {
        Self { handle, must_delete: false }
    }

    pub fn new(
        ty: IntegralType,
        size: usize,
        is_const: bool,
        is_refable: bool,
        type_id: &str,
        type_name: &str,
        script_callback_type: Option<ScriptCallbackType>,
        primary_type: Option<ObjectType>,
        secondary_type: Option<ObjectType>
    ) -> Self {
        let type_id_c = CString::new(type_id).unwrap();
        let type_name_c = CString::new(type_name).unwrap();
        Self {
            handle: unsafe { argus_object_type_new(
                ty.into(),
                size,
                is_const,
                is_refable,
                type_id_c.as_ptr(),
                type_name_c.as_ptr(),
                script_callback_type.map(|t| t.handle).unwrap_or(ptr::null_mut()),
                primary_type.map(|t| t.handle).unwrap_or(ptr::null_mut()),
                secondary_type.map(|t| t.handle).unwrap_or(ptr::null_mut()),
            ) },
            must_delete: true,
        }
    }

    pub fn get_type(&self) -> IntegralType {
        unsafe { IntegralType::unchecked_transmute_from(argus_object_type_get_type(self.handle)) }
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

    pub fn get_type_name(&self) -> String {
        unsafe {
            CStr::from_ptr(argus_object_type_get_type_name(self.handle))
                .to_string_lossy().to_string()
        }
    }

    pub fn get_callback_type(&self) -> Option<ScriptCallbackType> {
        unsafe {
            argus_object_type_get_callback_type(self.handle).cast_mut().as_mut()
                .map(|p| ScriptCallbackType::of(ptr::from_mut(p)))
        }
    }

    pub fn get_primary_type(&self) -> Option<ObjectType> {
        unsafe {
            argus_object_type_get_primary_type(self.handle).cast_mut().as_mut()
                .map(|p| ObjectType::of(ptr::from_mut(p)))
        }
    }

    pub fn get_secondary_type(&self) -> Option<ObjectType> {
        unsafe {
            argus_object_type_get_secondary_type(self.handle).cast_mut().as_mut()
                .map(|p| ObjectType::of(ptr::from_mut(p)))
        }
    }
}

impl Drop for ObjectType {
    fn drop(&mut self) {
        if self.must_delete {
            unsafe { argus_object_type_delete(self.handle); }
        }
    }
}

pub struct ScriptCallbackType {
    handle: argus_script_callback_type_t,
}

impl ScriptCallbackType {
    pub fn of(handle: argus_script_callback_type_t) -> Self {
        Self { handle }
    }
}

pub struct ObjectWrapper {
    handle: argus_object_wrapper_t,
}

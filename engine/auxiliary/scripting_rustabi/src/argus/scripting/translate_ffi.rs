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

use std::ffi::{CStr, CString};
use std::ptr;
use argus_scripting_bind::*;
use crate::argus::scripting::*;
use crate::scripting_cabi::*;

fn to_ffi_integral_type(ty: IntegralType) -> FfiIntegralType {
    match ty {
        IntegralType::Empty => FfiIntegralType::Void,
        IntegralType::Int8 |
        IntegralType::Int16 |
        IntegralType::Int32 |
        IntegralType::Int64 |
        IntegralType::Int128 => FfiIntegralType::Integer,
        IntegralType::Uint8 |
        IntegralType::Uint16 |
        IntegralType::Uint32 |
        IntegralType::Uint64 |
        IntegralType::Uint128 => FfiIntegralType::UInteger,
        IntegralType::Float32 |
        IntegralType::Float64 => FfiIntegralType::Float,
        IntegralType::Boolean => FfiIntegralType::Boolean,
        IntegralType::String => FfiIntegralType::String,
        IntegralType::Reference |
        IntegralType::MutReference => FfiIntegralType::Pointer,
        IntegralType::Vec => FfiIntegralType::Vector,
        IntegralType::Result => FfiIntegralType::Result,
        IntegralType::Object => FfiIntegralType::Struct,
        IntegralType::Enum => FfiIntegralType::Enum,
    }
}

fn from_ffi_integral_type(ty: FfiIntegralType, size: usize, is_const: bool) -> IntegralType {
    match ty {
        FfiIntegralType::Void => IntegralType::Empty,
        FfiIntegralType::Integer |
        FfiIntegralType::Enum => match size {
            1 => IntegralType::Int8,
            2 => IntegralType::Int16,
            4 => IntegralType::Int32,
            8 => IntegralType::Int64,
            16 => IntegralType::Int128,
            _ => panic!("Unhandled integer width {size}")
        }
        FfiIntegralType::UInteger => match size {
            1 => IntegralType::Uint8,
            2 => IntegralType::Uint16,
            4 => IntegralType::Uint32,
            8 => IntegralType::Uint64,
            16 => IntegralType::Uint128,
            _ => panic!("Unhandled integer width {size}")
        }
        FfiIntegralType::Float => match size {
            4 => IntegralType::Float32,
            8 => IntegralType::Float64,
            _ => panic!("Unhandled floating-point width {size}")
        }
        FfiIntegralType::Boolean => IntegralType::Boolean,
        FfiIntegralType::String => IntegralType::String,
        FfiIntegralType::Struct => IntegralType::Object,
        FfiIntegralType::Pointer => if is_const {
            IntegralType::Reference
        } else {
            IntegralType::MutReference
        },
        FfiIntegralType::Callback => todo!(),
        FfiIntegralType::Type => todo!(),
        FfiIntegralType::Vector => IntegralType::Vec,
        FfiIntegralType::Vectorref => IntegralType::Vec,
        FfiIntegralType::Result => IntegralType::Result,
    }
}

pub fn to_ffi_obj_type(ty: &ObjectType) -> FfiObjectType {
    let prim_type = ty.primary_type.as_ref().map(|prim| to_ffi_obj_type(prim));
    let sec_type = ty.secondary_type.as_ref().map(|sec| to_ffi_obj_type(sec));

    FfiObjectType::new(
        to_ffi_integral_type(ty.ty),
        ty.size,
        ty.is_const,
        ty.get_is_refable().unwrap_or(false),
        ty.type_id.map(|id| format!("{:?}", id)),
        None, //TODO
        prim_type,
        sec_type,
        ty.copy_ctor.map(|getter| getter.fn_ptr),
        ty.move_ctor.map(|getter| getter.fn_ptr),
    )
}

pub fn from_ffi_obj_type(ty: &FfiObjectType) -> ObjectType {
    let is_refable = ty.get_is_refable();

    ObjectType {
        ty: from_ffi_integral_type(ty.get_type(), ty.get_size(), ty.get_is_const()),
        size: ty.get_size(),
        is_const: ty.get_is_const(),
        is_refable: Some(is_refable),
        is_refable_getter: None,
        type_id: None,
        type_name: ty.get_type_name(),
        primary_type: ty.get_primary_type().map(|pt| Box::new(from_ffi_obj_type(&pt))),
        secondary_type: ty.get_secondary_type().map(|st| Box::new(from_ffi_obj_type(&st))),
        copy_ctor: None,
        move_ctor: None,
        dtor: None,
    }
}

pub fn to_ffi_obj_wrapper(obj: WrappedObject) ->
    Result<FfiObjectWrapper, ReflectiveArgumentsError> {
    unsafe {
        let obj_type_ffi = to_ffi_obj_type(&obj.ty);

        let res = argus_create_object_wrapper(
            obj_type_ffi.handle,
            obj.get_raw_ptr().cast(),
            obj.buffer_size,
        );
        if res.is_err {
            let reason = CStr::from_ptr(argus_reflective_args_error_get_reason(res.err));
            Err(ReflectiveArgumentsError { reason: reason.to_str().unwrap().to_string() })
        } else {
            Ok(FfiObjectWrapper::of(res.val))
        }
    }
}

fn from_ffi_obj_wrapper(obj: FfiObjectWrapper) -> WrappedObject {
    unsafe {
        let ty = from_ffi_obj_type(&FfiObjectType::of(argus_object_wrapper_get_type(obj.handle)));
        WrappedObject::from_ptr(&ty, obj.get_buffer())
    }
}

pub fn call_proxied_fn(
    f: &ProxiedNativeFunction,
    params: &[argus_object_wrapper_t]
) -> ArgusObjectWrapperOrReflectiveArgsError {
    let params_vec =
        params.iter()
            .map(|p| from_ffi_obj_wrapper(FfiObjectWrapper::of(*p)))
            .collect::<Vec<_>>();

    let retval_opt = f(&params_vec);
    match retval_opt {
        Ok(retval) => {
            match to_ffi_obj_wrapper(retval) {
                Ok(retval_ffi) => {
                    let res = ArgusObjectWrapperOrReflectiveArgsError {
                        is_err: false,
                        val: retval_ffi.handle,
                        err: ptr::null_mut(),
                    };
                    Box::leak(Box::new(retval_ffi)); //TODO
                    res
                }
                Err(e) => {
                    let reason_c = CString::new(e.reason).unwrap();
                    ArgusObjectWrapperOrReflectiveArgsError {
                        is_err: true,
                        val: ptr::null_mut(),
                        err: unsafe { argus_reflective_args_error_new(reason_c.as_ptr()) },
                    }
                }
            }
        },
        Err(e) => {
            let reason_c = CString::new(e.reason).unwrap();
            ArgusObjectWrapperOrReflectiveArgsError {
                is_err: true,
                val: ptr::null_mut(),
                err: unsafe { argus_reflective_args_error_new(reason_c.as_ptr()) },
            }
        }
    }
}

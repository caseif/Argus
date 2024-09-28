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

use std::any::TypeId;
use std::{ffi, mem, ptr};
use std::alloc::{alloc_zeroed, dealloc, Layout};
use std::ffi::{CStr, CString};
use std::fmt::Formatter;
use std::ptr::{slice_from_raw_parts, slice_from_raw_parts_mut};
use quote::{quote, ToTokens};
use serde::{Deserialize, Deserializer, Serialize, Serializer};
use serde::de::{Error, Visitor};

pub type ProxiedNativeFunction = fn(params: Vec<WrappedObject>) -> WrappedObject;

#[derive(Debug, Clone, Copy, PartialEq, Deserialize, Serialize)]
pub enum IntegralType {
    Empty,
    Int8,
    Int16,
    Int32,
    Int64,
    Int128,
    Uint8,
    Uint16,
    Uint32,
    Uint64,
    Uint128,
    Float32,
    Float64,
    Boolean,
    String,
    ConstReference,
    MutReference,
    ConstPointer,
    MutPointer,
    Vector,
    Result,
    Object,
}

macro_rules! fn_wrapper {
    ($struct_name:ident, $fn_sig:ty) => {
        #[derive(Clone, Copy)]
        pub struct $struct_name {
            pub fn_ptr: $fn_sig,
        }

        impl $struct_name {
            pub fn of(f: $fn_sig) -> Self {
                Self { fn_ptr: f }
            }
        }

        impl<'de> Deserialize<'de> for $struct_name {
            #[allow(clippy::needless_return)]
            fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
            where
                D: Deserializer<'de>
            {
                struct FnPointerVisitor;

                impl<'de> Visitor<'de> for FnPointerVisitor {
                    type Value = $fn_sig;

                    fn expecting(&self, formatter: &mut Formatter) -> std::fmt::Result {
                        formatter.write_str(stringify!($fn_sig))
                    }

                    #[cfg(target_pointer_width = "32")]
                    fn visit_u32<E>(self, v: u32) -> Result<Self::Value, E>
                    where
                        E: Error,
                    {
                        Ok(unsafe { mem::transmute::<u32, $fn_sig>(v) })
                    }

                    #[cfg(target_pointer_width = "64")]
                    fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
                    where
                        E: Error,
                    {
                        Ok(unsafe { mem::transmute::<u64, $fn_sig>(v) })
                    }
                }

                #[cfg(target_pointer_width = "32")]
                return Ok($struct_name {
                    getter_fn: deserializer.deserialize_u32(FnPointerVisitor)?
                });
                #[cfg(target_pointer_width = "64")]
                return Ok($struct_name {
                    fn_ptr: deserializer.deserialize_u64(FnPointerVisitor)?
                });
            }
        }

        impl Serialize for $struct_name {
            #[allow(clippy::needless_return)]
            #[allow(clippy::transmutes_expressible_as_ptr_casts)]
            fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
            where
                S: Serializer
            {
                #[cfg(target_pointer_width = "32")]
                return serializer.serialize_u32(
                    unsafe { mem::transmute::<$fn_sig, u32>(self.fn_ptr) }
                );
                #[cfg(target_pointer_width = "64")]
                return serializer.serialize_u64(
                    unsafe { mem::transmute::<$fn_sig, u64>(self.fn_ptr) }
                );
            }
        }
    }
}

fn_wrapper!(IsRefableGetter, fn() -> bool);
fn_wrapper!(TypeIdGetter, fn() -> TypeId);

#[derive(Clone, Copy, Debug)]
pub enum FunctionType {
    Global,
    MemberStatic,
    MemberInstance,
    Extension,
}

impl ToTokens for FunctionType {
    fn to_tokens(&self, tokens: &mut proc_macro2::TokenStream) {
        match self {
            FunctionType::Global =>
                tokens.extend(quote! { ::argus_scripting_bind::FunctionType::Global }),
            FunctionType::MemberStatic =>
                tokens.extend(quote! { ::argus_scripting_bind::FunctionType::MemberStatic }),
            FunctionType::MemberInstance =>
                tokens.extend(quote! { ::argus_scripting_bind::FunctionType::MemberInstance }),
            FunctionType::Extension =>
                tokens.extend(quote! { ::argus_scripting_bind::FunctionType::Extension }),
        }
    }
}

#[derive(Clone, Deserialize, Serialize)]
pub struct ObjectType {
    pub ty: IntegralType,
    pub size: usize,
    pub is_const: bool,
    pub is_refable: Option<IsRefableGetter>,
    pub primary_type: Option<Box<ObjectType>>,
    pub secondary_type: Option<Box<ObjectType>>,
    pub type_id: Option<TypeIdGetter>,
}

pub const EMPTY_TYPE: ObjectType = ObjectType {
    ty: IntegralType::Empty,
    size: 0,
    is_const: false,
    is_refable: None,
    primary_type: None,
    secondary_type: None,
    type_id: None,
};

pub trait Wrappable<'a, U : Into<Self> = Self> : Sized {
    type InternalFormat;

    fn get_object_type() -> ObjectType;

    fn unwrap_from(wrapper: WrappedObject) -> U;

    fn wrap_into(self, wrapper: &mut WrappedObject);

    fn get_required_buffer_size(&self) -> usize {
        size_of::<Self>()
    }
}

macro_rules! make_wrappable {
    ($ty:ty, $enum_var:ident) => (
        impl<'a> Wrappable<'a> for $ty {
            type InternalFormat = Self;

            fn get_object_type() -> ObjectType {
                ObjectType {
                    ty: IntegralType::$enum_var,
                    size: size_of::<Self>(),
                    is_const: false,
                    is_refable: None,
                    primary_type: None,
                    secondary_type: None,
                    type_id: None,
                }
            }

            fn wrap_into(self, wrapper: &mut WrappedObject) {
                unsafe { wrapper.store_value(self); }
            }

            fn unwrap_from(wrapper: WrappedObject) -> Self {
                assert_eq!(wrapper.ty.ty, IntegralType::$enum_var, "Wrong object type");

                unsafe { *wrapper.get_ptr::<Self>() }
            }
        }
    )
}

make_wrappable!(i8, Int8);
make_wrappable!(i16, Int16);
make_wrappable!(i32, Int32);
make_wrappable!(i64, Int64);
make_wrappable!(u8, Uint8);
make_wrappable!(u16, Uint16);
make_wrappable!(u32, Uint32);
make_wrappable!(u64, Uint64);
make_wrappable!(f32, Float32);
make_wrappable!(f64, Float64);
make_wrappable!(bool, Boolean);
make_wrappable!((), Empty);

impl<'a> Wrappable<'a> for String {
    type InternalFormat = &'a [u8];

    fn get_object_type() -> ObjectType {
        ObjectType {
            ty: IntegralType::String,
            size: 0,
            is_const: false,
            is_refable: None,
            primary_type: None,
            secondary_type: None,
            type_id: None,
        }
    }

    fn unwrap_from(mut wrapper: WrappedObject) -> Self {
        assert_eq!(wrapper.ty.ty, IntegralType::String, "Wrong object type");

        unsafe {
            CString::from(CStr::from_ptr((*wrapper.get_mut_ptr::<Self>()).as_ptr().cast()))
                    .into_string().unwrap()
        }
    }

    fn wrap_into(self, wrapped: &mut WrappedObject) {
        let cstr = CString::new(self.as_str()).unwrap();

        unsafe { wrapped.copy_from_slice(cstr.as_bytes_with_nul()); }

        wrapped.is_initialized = true;
    }

    fn get_required_buffer_size(&self) -> usize {
        CString::new(self.as_str()).unwrap().count_bytes() + 1
    }
}

impl<'a> Wrappable<'a> for &'a str {
    type InternalFormat = &'a [u8];

    fn get_object_type() -> ObjectType {
        ObjectType {
            ty: IntegralType::String,
            size: 0,
            is_const: false,
            is_refable: None,
            primary_type: None,
            secondary_type: None,
            type_id: None,
        }
    }

    fn unwrap_from(mut wrapper: WrappedObject) -> Self {
        assert_eq!(wrapper.ty.ty, IntegralType::String, "Wrong object type");

        unsafe {
            CStr::from_ptr((*wrapper.get_mut_ptr::<Self>()).as_ptr().cast()).to_str().unwrap()
        }
    }

    fn wrap_into(self, wrapped: &mut WrappedObject) {
        let cstr = CString::new(self).unwrap();

        unsafe { wrapped.copy_from_slice(cstr.as_bytes_with_nul()); }

        wrapped.is_initialized = true;
    }

    fn get_required_buffer_size(&self) -> usize {
        CString::new(*self).unwrap().count_bytes() + 1
    }
}

const WRAPPED_OBJ_BUF_CAP: usize = 64;

pub struct WrappedObject {
    pub ty: ObjectType,
    pub data: [u8; WRAPPED_OBJ_BUF_CAP],
    pub is_on_heap: bool,
    pub buffer_size: usize,
    is_initialized: bool,
    drop_fn: fn(&mut WrappedObject),
}

impl Drop for WrappedObject {
    fn drop(&mut self) {
        (self.drop_fn)(self);
        if self.is_on_heap {
            unsafe {
                dealloc(
                    self.get_raw_heap_ptr_mut(),
                    Layout::from_size_align(self.buffer_size, 8).unwrap()
                );
            }
        }
    }
}

impl WrappedObject {
    pub const BUFFER_CAPACITY: usize = WRAPPED_OBJ_BUF_CAP;

    pub fn wrap<'a, T: Wrappable<'a>>(val: T) -> Self {
        let size = val.get_required_buffer_size();

        let mut wrapper = WrappedObject {
            ty: T::get_object_type(),
            data: unsafe { mem::zeroed() },
            is_on_heap: false,
            buffer_size: size,
            is_initialized: false,
            drop_fn: |wrapper: &mut WrappedObject|
                unsafe { ptr::drop_in_place::<T::InternalFormat>(wrapper.get_heap_ptr_mut::<T>()) },
        };

        if size > WrappedObject::BUFFER_CAPACITY {
            let heap_ptr: *mut T::InternalFormat =
                unsafe {
                    alloc_zeroed(Layout::from_size_align(wrapper.buffer_size, 8).unwrap()).cast()
                };
            unsafe {
                // copy pointer address to internal buffer
                ptr::write(wrapper.data.as_mut_ptr().cast(), heap_ptr);
            }
            wrapper.is_on_heap = true;
        } else {
            wrapper.is_on_heap = false;
        }

        val.wrap_into(&mut wrapper);

        wrapper
    }

    pub fn unwrap<'a, T : Wrappable<'a>>(self) -> T {
        assert!(size_of::<Self>() <= WrappedObject::BUFFER_CAPACITY);
        T::unwrap_from(self)
    }

    pub fn get_ptr<'a, T : Wrappable<'a>>(&self) -> *const T::InternalFormat {
        if self.is_on_heap {
            self.get_heap_ptr::<T>()
        } else {
            self.data.as_ptr().cast()
        }
    }

    pub fn get_mut_ptr<'a, T : Wrappable<'a>>(&mut self) -> *mut T::InternalFormat {
        if self.is_on_heap {
            self.get_heap_ptr_mut::<T>()
        } else {
            self.data.as_mut_ptr().cast()
        }
    }

    pub fn get_data_as_slice(&self) -> &[u8] {
        unsafe { &*slice_from_raw_parts(self.data.as_ptr().cast(), self.buffer_size) }
    }

    pub fn get_data_as_mut_slice(&mut self) -> &mut [u8] {
        unsafe { &mut *slice_from_raw_parts_mut(self.data.as_mut_ptr().cast(), self.buffer_size) }
    }

    pub unsafe fn store_value<'a, T : Wrappable<'a>>(&mut self, val: T) {
        assert!(size_of::<T>() <= self.buffer_size);

        val.wrap_into(self);
        self.is_initialized = true;
    }

    pub unsafe fn copy_from_slice(&mut self, src: &[u8]) {
        self.copy_from(src.as_ptr().cast(), src.len());
    }

    unsafe fn copy_from(&mut self, src: *const (), size: usize) {
        assert!(size <= self.buffer_size);

        ptr::copy_nonoverlapping(src, self.get_mut_ptr::<()>(), size);
    }

    fn get_heap_ptr<'a, T : Wrappable<'a>>(&self) -> *const T::InternalFormat {
        assert!(size_of::<*const T::InternalFormat>() <= self.buffer_size);
        self.get_raw_heap_ptr().cast()
    }

    fn get_heap_ptr_mut<'a, T : Wrappable<'a>>(&mut self) -> *mut T::InternalFormat {
        assert!(size_of::<*mut T::InternalFormat>() <= self.buffer_size);
        self.get_raw_heap_ptr_mut().cast()
    }

    fn get_raw_heap_ptr(&self) -> *mut u8 {
        assert!(self.is_on_heap);
        unsafe { *self.data.as_ptr().cast() }
    }

    fn get_raw_heap_ptr_mut(&mut self) -> *mut u8 {
        assert!(self.is_on_heap);
        unsafe { *self.data.as_ptr().cast() }
    }
}

pub struct BoundStructInfo {
    pub name: &'static str,
    pub type_id: fn() -> TypeId,
    pub size: usize,
}

pub struct BoundFieldInfo {
    pub containing_type: fn() -> TypeId,
    pub name: &'static str,
    pub type_serial: &'static str,
    pub size: usize,
    pub accessor: unsafe fn(*mut ffi::c_void) -> *mut ffi::c_void,
    pub mutator: unsafe fn(*mut ffi::c_void, *const ffi::c_void) -> (),
}

pub struct BoundFunctionInfo {
    pub name: &'static str,
    pub ty: FunctionType,
    pub param_type_serials: &'static [&'static str],
    pub return_type_serial: &'static str,
    pub proxy: ProxiedNativeFunction,
}

pub struct BoundEnumInfo {
    pub name: &'static str,
    pub type_id: fn() -> TypeId,
    pub width: usize,
    pub values: &'static [(&'static str, fn() -> i64)],
}

impl BoundEnumInfo {
    pub const fn new(
        name: &'static str,
        type_id: fn() -> TypeId,
        width: usize,
        values: &'static [(&'static str, fn() -> i64)],
    ) -> Self {
        Self { name, type_id, width, values }
    }
}

inventory::collect!(BoundStructInfo);
inventory::collect!(BoundFieldInfo);
inventory::collect!(BoundFunctionInfo);
inventory::collect!(BoundEnumInfo);

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
use std::{ffi, mem, ptr, slice};
use std::alloc::{alloc_zeroed, dealloc, Layout};
use std::ffi::{CStr, CString};
use std::ptr::{slice_from_raw_parts, slice_from_raw_parts_mut};
use std::sync::{Arc, Mutex};
use num_enum::{IntoPrimitive, TryFromPrimitive};
use quote::{quote, ToTokens};
use serde::{Deserialize, Serialize, Serializer};

pub type IsRefableGetter = fn() -> bool;
pub type TypeIdGetter = fn() -> TypeId;

pub trait ScriptCallbackRef: Send {
    fn call(&self, params: Vec<WrappedObject>) -> Result<WrappedObject, ScriptInvocationError>;
}

pub type ProxiedNativeFunction =
    fn(Vec<WrappedObject>) -> Result<WrappedObject, ReflectiveArgumentsError>;

pub type ScriptCallbackEntryPoint = fn(
    params: Vec<WrappedObject>,
    userdata: Arc<Mutex<dyn ScriptCallbackRef>>,
) -> Result<WrappedObject, ScriptInvocationError>;

//#[derive(Clone, Copy)]
#[derive(Clone)]
pub struct WrappedScriptCallback {
    pub entry_point: ScriptCallbackEntryPoint,
    pub userdata: Arc<Mutex<dyn ScriptCallbackRef>>,
}

pub type FfiCopyCtor = unsafe extern "C" fn(dst: *mut (), src: *const ());
pub type FfiDtor = unsafe extern "C" fn(target: *mut ());

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
    Reference,
    Vec,
    VecRef,
    Result,
    Object,
    Enum,
    Callback,
}

impl IntegralType {
    pub fn is_int(&self) -> bool {
        self == &IntegralType::Int8 ||
        self == &IntegralType::Int16 ||
        self == &IntegralType::Int32 ||
        self == &IntegralType::Int64 ||
        self == &IntegralType::Int128
    }

    pub fn is_uint(&self) -> bool {
        self == &IntegralType::Uint8 ||
            self == &IntegralType::Uint16 ||
            self == &IntegralType::Uint32 ||
            self == &IntegralType::Uint64 ||
            self == &IntegralType::Uint128
    }

    pub fn is_float(&self) -> bool {
        self == &IntegralType::Float32 ||
            self == &IntegralType::Float64
    }

    pub fn is_integral(&self) -> bool {
        self.is_int() ||
            self.is_uint() ||
            self == &IntegralType::Enum
    }
}

macro_rules! fn_wrapper {
    ($struct_name:ident, $fn_sig:ty) => {
        #[derive(::std::clone::Clone, ::std::marker::Copy, ::std::fmt::Debug)]
        pub struct $struct_name {
            pub fn_ptr: $fn_sig,
        }

        impl $struct_name {
            pub fn of(f: $fn_sig) -> Self {
                Self { fn_ptr: f }
            }
        }

        impl<'de> ::serde::de::Deserialize<'de> for $struct_name {
            #[allow(clippy::needless_return)]
            fn deserialize<D>(deserializer: D) -> ::std::result::Result<Self, D::Error>
            where
                D: ::serde::de::Deserializer<'de>
            {
                struct FnPointerVisitor;

                impl<'de> ::serde::de::Visitor<'de> for FnPointerVisitor {
                    type Value = $fn_sig;

                    fn expecting(&self, formatter: &mut ::std::fmt::Formatter)
                    -> ::std::fmt::Result {
                        formatter.write_str(stringify!($fn_sig))
                    }

                    #[cfg(target_pointer_width = "32")]
                    fn visit_u32<E>(self, v: u32) -> ::std::result::Result<Self::Value, E>
                    where
                        E: ::serde::de::Error,
                    {
                        Ok(unsafe { ::std::mem::transmute::<u32, $fn_sig>(v) })
                    }

                    #[cfg(target_pointer_width = "64")]
                    fn visit_u64<E>(self, v: u64) -> ::std::result::Result<Self::Value, E>
                    where
                        E: ::serde::de::Error,
                    {
                        Ok(unsafe { ::std::mem::transmute::<u64, $fn_sig>(v) })
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

fn_wrapper!(IsRefableGetterWrapper, IsRefableGetter);
fn_wrapper!(TypeIdGetterWrapper, TypeIdGetter);
fn_wrapper!(CopyCtorWrapper, FfiCopyCtor);
fn_wrapper!(DtorWrapper, FfiDtor);

#[derive(Clone, Copy, Debug, IntoPrimitive, PartialEq, PartialOrd, TryFromPrimitive)]
#[repr(u32)]
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

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct ObjectType {
    pub ty: IntegralType,
    pub size: usize,
    pub is_const: bool,
    pub is_refable: Option<bool>,
    pub is_refable_getter: Option<IsRefableGetterWrapper>,
    #[serde(skip)]
    pub type_id: Option<String>,
    pub type_name: Option<String>,
    pub primary_type: Option<Box<ObjectType>>,
    pub secondary_type: Option<Box<ObjectType>>,
    pub callback_info: Option<Box<CallbackInfo>>,
    pub copy_ctor: Option<CopyCtorWrapper>,
    pub dtor: Option<DtorWrapper>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct CallbackInfo {
    pub param_types: Vec<ObjectType>,
    pub return_type: ObjectType,
}

impl ObjectType {
    pub fn empty() -> Self {
        Self {
            ty: IntegralType::Empty,
            size: 0,
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            type_id: None,
            type_name: None,
            primary_type: None,
            secondary_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        }
    }

    pub fn get_is_refable(&self) -> Option<bool> {
        if let Some(refable) = self.is_refable {
            Some(refable)
        } else if let Some(getter) = self.is_refable_getter {
            Some((getter.fn_ptr)())
        } else {
            None
        }
    }
}

pub const EMPTY_TYPE: ObjectType = ObjectType {
    ty: IntegralType::Empty,
    size: 0,
    is_const: false,
    is_refable: None,
    is_refable_getter: None,
    type_id: None,
    type_name: None,
    primary_type: None,
    secondary_type: None,
    callback_info: None,
    copy_ctor: None,
    dtor: None,
};

pub trait ScriptBound: Sized {
    fn get_object_type() -> ObjectType;
}

macro_rules! impl_wrappable {
    ($ty:ty, $enum_var:ident) => (
        impl $crate::ScriptBound for $ty {
            fn get_object_type() -> $crate::ObjectType {
                ObjectType {
                    ty: $crate::IntegralType::$enum_var,
                    size: ::std::mem::size_of::<Self>(),
                    is_const: false,
                    is_refable: None,
                    is_refable_getter: None,
                    type_id: None,
                    type_name: None,
                    primary_type: None,
                    secondary_type: None,
                    callback_info: None,
                    copy_ctor: None,
                    dtor: None,
                }
            }
        }

        impl Wrappable for $ty {
            type InternalFormat = Self;

            fn wrap_into(self, wrapper: &mut $crate::WrappedObject)
            where
                Self: $crate::Wrappable<InternalFormat = Self> + ::std::clone::Clone {
                if ::std::mem::size_of::<Self>() > 0 {
                    unsafe { wrapper.store_value::<Self>(self) }
                }
            }

            fn get_required_buffer_size(&self) -> usize
            where
                Self: $crate::Wrappable<InternalFormat = Self> + ::std::clone::Clone {
                ::std::mem::size_of::<Self>()
            }

            fn unwrap_as_value(wrapper: &$crate::WrappedObject) -> Self
            where
                Self: $crate::Wrappable<InternalFormat = Self> + ::std::clone::Clone {
                // SAFETY: the implementation provided by this macro guarantees
                //         that Self::InternalFormat == Self
                unsafe { <Self::InternalFormat as Clone>::clone(&*wrapper.get_ptr::<Self>()) }
            }
        }
    )
}

pub trait Wrappable : ScriptBound {
    type InternalFormat;

    fn wrap_into(self, wrapper: &mut WrappedObject);

    fn get_required_buffer_size(&self) -> usize {
        size_of::<Self::InternalFormat>()
    }

    fn unwrap_as_value(wrapper: &WrappedObject) -> Self;
}

impl_wrappable!(i8, Int8);
impl_wrappable!(i16, Int16);
impl_wrappable!(i32, Int32);
impl_wrappable!(i64, Int64);
impl_wrappable!(u8, Uint8);
impl_wrappable!(u16, Uint16);
impl_wrappable!(u32, Uint32);
impl_wrappable!(u64, Uint64);
impl_wrappable!(f32, Float32);
impl_wrappable!(f64, Float64);
impl_wrappable!(bool, Boolean);
impl_wrappable!((), Empty);

impl ScriptBound for String {
    fn get_object_type() -> ObjectType {
        ObjectType {
            ty: IntegralType::String,
            size: 0,
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            type_id: None,
            type_name: None,
            primary_type: None,
            secondary_type: None,
            callback_info: None,
            copy_ctor: Some(CopyCtorWrapper::of(copy_string)),
            dtor: Some(DtorWrapper::of(drop_string)),
        }
    }
}

impl Wrappable for String {
    type InternalFormat = ffi::c_char;

    fn wrap_into(self, wrapper: &mut WrappedObject) {
        let cstr = CString::new(self.as_str()).unwrap();

        unsafe { wrapper.copy_from_slice(cstr.as_bytes_with_nul()); }

        wrapper.is_populated = true;
    }

    fn get_required_buffer_size(&self) -> usize {
        CString::new(self.as_str()).unwrap().count_bytes() + 1
    }

    fn unwrap_as_value(wrapper: &WrappedObject) -> Self {
        assert_eq!(wrapper.ty.ty, IntegralType::String, "Wrong object type");
        assert!(wrapper.is_populated);

        unsafe {
            let char_ptr = wrapper.get_ptr::<Self>();
            let c_str = CStr::from_ptr(char_ptr);
            c_str.to_str().unwrap().to_string()
        }
    }
}

impl<'a> ScriptBound for &'a str {
    fn get_object_type() -> ObjectType {
        ObjectType {
            ty: IntegralType::String,
            size: 0,
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            type_id: None,
            type_name: None,
            primary_type: None,
            secondary_type: None,
            callback_info: None,
            copy_ctor: Some(CopyCtorWrapper::of(copy_string)),
            dtor: None,
        }
    }
}

impl<'a> Wrappable for &'a str {
    type InternalFormat = ffi::c_char;

    fn wrap_into(self, wrapper: &mut WrappedObject) {
        let cstr = CString::new(self).unwrap();

        unsafe { wrapper.copy_from_slice(cstr.as_bytes_with_nul()); }

        wrapper.is_populated = true;
    }

    fn get_required_buffer_size(&self) -> usize {
        CString::new(*self).unwrap().count_bytes() + 1
    }

    fn unwrap_as_value(wrapper: &WrappedObject) -> Self {
        assert_eq!(wrapper.ty.ty, IntegralType::String, "Wrong object type");
        assert!(wrapper.is_populated);

        unsafe {
            let char_ptr = wrapper.get_ptr::<Self>();
            let c_str = CStr::from_ptr(char_ptr);
            c_str.to_str().unwrap()
        }
    }
}

impl<T: ScriptBound> ScriptBound for Vec<T> {
    fn get_object_type() -> ObjectType {
        ObjectType {
            ty: IntegralType::Vec,
            size: 0,
            is_const: true,
            is_refable: None,
            is_refable_getter: None,
            type_id: None,
            type_name: None,
            primary_type: Some(Box::new(T::get_object_type())),
            secondary_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        }
    }
}

impl<T: Wrappable> Wrappable for Vec<T> {
    type InternalFormat = i32;

    fn wrap_into(self, _wrapper: &mut WrappedObject) {
        todo!()
    }

    fn unwrap_as_value(_wrapper: &WrappedObject) -> Self {
        todo!()
    }
}

impl<T: ScriptBound> ScriptBound for &T {
    fn get_object_type() -> ObjectType {
        let base_type = T::get_object_type();
        ObjectType {
            ty: IntegralType::Reference,
            size: size_of::<*const ()>(),
            is_const: false, //TODO: this totally breaks safety in bound functions
            is_refable: None,
            is_refable_getter: None,
            type_id: base_type.type_id,
            type_name: base_type.type_name,
            primary_type: Some(Box::new(T::get_object_type())),
            secondary_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        }
    }
}

impl<T: ScriptBound> Wrappable for &T {
    type InternalFormat = *const T;

    fn wrap_into(self, wrapper: &mut WrappedObject) {
        assert_eq!(wrapper.ty.ty, IntegralType::Reference, "Wrong object type");

        unsafe { *wrapper.get_mut_ptr::<Self>() = self; }
        wrapper.is_populated = true;
    }

    fn unwrap_as_value(wrapper: &WrappedObject) -> Self {
        assert_eq!(wrapper.ty.ty, IntegralType::Reference, "Wrong object type");
        assert!(wrapper.is_populated);

        unsafe { &**wrapper.get_ptr::<Self>() }
    }
}

impl<T: ScriptBound> ScriptBound for &mut T {
    fn get_object_type() -> ObjectType {
        let base_type = T::get_object_type();
        ObjectType {
            ty: IntegralType::Reference,
            size: size_of::<*const ()>(),
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            type_id: base_type.type_id,
            type_name: base_type.type_name,
            primary_type: Some(Box::new(T::get_object_type())),
            secondary_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        }
    }
}

impl<T: ScriptBound> Wrappable for &mut T {
    type InternalFormat = *mut T;

    fn wrap_into(self, wrapper: &mut WrappedObject) {
        assert_eq!(wrapper.ty.ty, IntegralType::Reference, "Wrong object type");
        assert!(!wrapper.ty.is_const);

        unsafe { *wrapper.get_mut_ptr::<Self>() = self; }
        wrapper.is_populated = true;
    }

    fn unwrap_as_value(wrapper: &WrappedObject) -> Self {
        assert_eq!(wrapper.ty.ty, IntegralType::Reference, "Wrong object type");
        assert!(!wrapper.ty.is_const);
        assert!(wrapper.is_populated);

        unsafe { &mut **wrapper.get_ptr::<Self>() }
    }
}

const WRAPPED_OBJ_BUF_CAP: usize = 64;

pub struct WrappedObject {
    pub ty: ObjectType,
    pub data: [u8; WRAPPED_OBJ_BUF_CAP],
    pub is_on_heap: bool,
    pub buffer_size: usize,
    is_populated: bool,
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

    pub fn from_ptr(ty: &ObjectType, ffi_buf: &[u8]) -> Self {
        let mut wrapper = Self {
            ty: ty.clone(),
            data: unsafe { mem::zeroed() },
            is_on_heap: false,
            buffer_size: ffi_buf.len(),
            is_populated: false,
            drop_fn: |_| {},
        };

        wrapper.initialize_buffer(ffi_buf.len());

        if let Some(copy_ctor) = ty.copy_ctor {
            unsafe { (copy_ctor.fn_ptr)(wrapper.get_mut_ptr::<()>(), ffi_buf.as_ptr().cast()) }
        } else {
            unsafe { wrapper.copy_from_slice(ffi_buf) }
        }

        wrapper
    }

    pub fn wrap<T: Wrappable>(val: T) -> Self {
        let size = val.get_required_buffer_size();

        let mut wrapper = WrappedObject {
            ty: T::get_object_type(),
            data: unsafe { mem::zeroed() },
            is_on_heap: false,
            buffer_size: size,
            is_populated: false,
            drop_fn: |wrapper: &mut WrappedObject| {
                unsafe {
                    if wrapper.is_populated {
                        ptr::drop_in_place::<T::InternalFormat>(wrapper.get_mut_ptr::<T>())
                    }
                }
            },
        };

        wrapper.initialize_buffer(size);

        val.wrap_into(&mut wrapper);

        wrapper
    }

    pub fn new(ty: ObjectType, size: usize) -> Self {
        let mut wrapper = WrappedObject {
            ty,
            data: unsafe { mem::zeroed() },
            is_on_heap: false,
            buffer_size: size,
            is_populated: false,
            drop_fn: |_| {},
        };

        wrapper.initialize_buffer(size);

        wrapper
    }

    pub fn unwrap<T: Wrappable>(&self) -> T {
        <T as Wrappable>::unwrap_as_value(&self)
    }

    fn initialize_buffer(&mut self, size: usize) {
        if size > WrappedObject::BUFFER_CAPACITY {
            let heap_ptr: *mut u8 =
                unsafe {
                    alloc_zeroed(Layout::from_size_align(self.buffer_size, 8).unwrap()).cast()
                };
            unsafe {
                // copy pointer address to internal buffer
                *self.data.as_mut_ptr().cast() = heap_ptr;
            }
            self.is_on_heap = true;
        } else {
            self.is_on_heap = false;
        }
    }

    pub fn get<T: Wrappable>(&self) -> &T::InternalFormat {
        unsafe { &*self.get_ptr::<T>() }
    }

    pub fn get_mut<T: Wrappable>(&mut self) -> &mut T::InternalFormat {
        unsafe { &mut *self.get_mut_ptr::<T>() }
    }

    pub fn get_ptr<T: Wrappable>(&self) -> *const T::InternalFormat {
        if self.is_on_heap {
            self.get_heap_ptr::<T>()
        } else {
            self.data.as_ptr().cast()
        }
    }

    pub fn get_mut_ptr<T: Wrappable>(&mut self) -> *mut T::InternalFormat {
        if self.is_on_heap {
            self.get_heap_ptr_mut::<T>()
        } else {
            self.data.as_mut_ptr().cast()
        }
    }

    pub fn get_raw_ptr(&self) -> *const () {
        if self.is_on_heap {
            self.get_raw_heap_ptr().cast()
        } else {
            self.data.as_ptr().cast()
        }
    }

    pub fn get_raw_mut_ptr(&mut self) -> *mut () {
        if self.is_on_heap {
            self.get_raw_heap_ptr_mut().cast()
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

    pub unsafe fn store_value<T: Wrappable + Into<T::InternalFormat>>(&mut self, val: T) {
        assert!(size_of::<T>() <= self.buffer_size);
        *self.get_mut_ptr::<T>() = val.into();
        self.is_populated = true;
    }

    pub unsafe fn store_internal<T>(&mut self, val: T) {
        assert!(size_of::<T>() <= self.buffer_size);
        if self.is_populated {
            *self.get_raw_mut_ptr().cast() = val;
        } else {
            ptr::write(self.get_raw_mut_ptr().cast(), val);
        }
        self.is_populated = true;
    }

    pub unsafe fn copy_from_slice(&mut self, src: &[u8]) {
        self.copy_from(src.as_ptr().cast(), src.len());
    }

    unsafe fn copy_from(&mut self, src: *const (), size: usize) {
        assert!(size <= self.buffer_size);

        let dst = self.get_mut_ptr::<u8>();
        ptr::copy_nonoverlapping(src.cast(), dst, size);

        self.is_populated = true;
    }

    fn get_heap_ptr<T: Wrappable>(&self) -> *const T::InternalFormat {
        assert!(size_of::<*const T::InternalFormat>() <= self.buffer_size);
        self.get_raw_heap_ptr().cast()
    }

    fn get_heap_ptr_mut<T: Wrappable>(&mut self) -> *mut T::InternalFormat {
        assert!(size_of::<*mut T::InternalFormat>() <= self.buffer_size);
        self.get_raw_heap_ptr_mut().cast()
    }

    fn get_raw_heap_ptr(&self) -> *const u8 {
        assert!(self.is_on_heap);
        unsafe { *self.data.as_ptr().cast() }
    }

    fn get_raw_heap_ptr_mut(&mut self) -> *mut u8 {
        assert!(self.is_on_heap);
        unsafe { *self.data.as_ptr().cast() }
    }
}

#[derive(Clone, Debug)]
pub struct ReflectiveArgumentsError {
    pub reason: String,
}

impl ReflectiveArgumentsError {
    pub fn new(reason: impl Into<String>) -> Self {
        Self { reason: reason.into() }
    }
}

#[derive(Clone, Debug)]
pub struct ScriptInvocationError {
    pub fn_name: String,
    pub message: String,
}

#[derive(Clone, Debug)]
pub struct BoundStructInfo {
    pub name: &'static str,
    pub type_id: fn() -> TypeId,
    pub size: usize,
    pub copy_ctor: Option<FfiCopyCtor>,
    pub dtor: Option<FfiDtor>,
}

#[derive(Clone, Debug)]
pub struct BoundFieldInfo {
    pub containing_type: fn() -> TypeId,
    pub name: &'static str,
    pub type_serial: &'static str,
    pub size: usize,
    pub accessor: fn(&WrappedObject, &ObjectType) -> WrappedObject,
    pub mutator: fn(&mut WrappedObject, &WrappedObject) -> (),
}

#[derive(Clone, Debug)]
pub struct BoundFunctionInfo {
    pub name: &'static str,
    pub ty: FunctionType,
    pub is_const: bool,
    pub param_names: &'static [&'static str],
    pub param_type_serials: &'static [&'static str],
    pub return_type_serial: &'static str,
    pub assoc_type: Option<TypeIdGetter>,
    pub proxy: ProxiedNativeFunction,
}

#[derive(Clone, Debug)]
pub struct BoundEnumInfo {
    pub name: &'static str,
    pub type_id: fn() -> TypeId,
    pub width: usize,
    pub values: &'static [(&'static str, fn() -> i64)],
}

pub unsafe extern "C" fn copy_string(dst: *mut (), src: *const ()) {
    let len = CStr::from_ptr(src.cast()).count_bytes() + 1;
    let src_slice = slice::from_raw_parts(src, len);
    let dst_slice = slice::from_raw_parts_mut(dst, len);
    dst_slice.copy_from_slice(src_slice);
}

pub unsafe extern "C" fn move_string(dst: *mut (), src: *mut ()) {
    copy_string(dst, src);
}

pub unsafe extern "C" fn drop_string(target: *mut ()) {
    ptr::drop_in_place(target.cast::<String>());
}

impl WrappedScriptCallback {
    pub unsafe fn call(
        callback: WrappedScriptCallback,
        params: Vec<WrappedObject>,
    ) -> Result<WrappedObject, ScriptInvocationError> {
        (callback.entry_point)(
            params,
            callback.userdata,
        )
    }
}

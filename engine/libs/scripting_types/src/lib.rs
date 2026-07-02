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

use std::any::{type_name, TypeId};
use std::{mem, ptr, slice};
use std::alloc::{alloc_zeroed, dealloc, Layout};
use std::cell::RefCell;
use std::ffi::{CStr, CString};
use std::mem::ManuallyDrop;
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
pub enum FundamentalType {
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

impl FundamentalType {
    pub fn is_int(&self) -> bool {
        self == &FundamentalType::Int8 ||
        self == &FundamentalType::Int16 ||
        self == &FundamentalType::Int32 ||
        self == &FundamentalType::Int64 ||
        self == &FundamentalType::Int128
    }

    pub fn is_uint(&self) -> bool {
        self == &FundamentalType::Uint8 ||
            self == &FundamentalType::Uint16 ||
            self == &FundamentalType::Uint32 ||
            self == &FundamentalType::Uint64 ||
            self == &FundamentalType::Uint128
    }

    pub fn is_float(&self) -> bool {
        self == &FundamentalType::Float32 ||
            self == &FundamentalType::Float64
    }

    pub fn is_integral(&self) -> bool {
        self.is_int() ||
            self.is_uint() ||
            self == &FundamentalType::Enum
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
    pub ty: FundamentalType,
    pub size: usize,
    pub is_const: bool,
    pub is_refable: Option<bool>,
    pub is_refable_getter: Option<IsRefableGetterWrapper>,
    #[serde(skip)]
    pub base_type_id: Option<String>,
    pub base_type_name: Option<String>,
    pub primary_type: Option<Box<ObjectType>>,
    pub secondary_type: Option<Box<ObjectType>>,
    pub synthetic_type: Option<String>,
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
            ty: FundamentalType::Empty,
            size: 0,
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            base_type_id: None,
            base_type_name: None,
            primary_type: None,
            secondary_type: None,
            synthetic_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        }
    }

    pub fn get_is_refable(&self) -> Option<bool> {
        if let Some(refable) = self.is_refable {
            Some(refable)
        } else {
            self.is_refable_getter.map(|getter| (getter.fn_ptr)())
        }
    }
}

pub trait ScriptBound: Sized {
    fn get_object_type() -> ObjectType;
}

pub unsafe trait WrappableUnderlying {
    /// # Safety
    /// `ptr` must point to a valid instance of `Self` of length `size` bytes.
    unsafe fn ptr_from_parts(ptr: *const (), size: usize) -> *const Self;
    /// # Safety
    /// `ptr` must point to a valid instance of `Self` of length `size` bytes.
    unsafe fn ptr_from_parts_mut(ptr: *mut (), size: usize) -> *mut Self;
}

unsafe impl<T: Sized> WrappableUnderlying for T {
    unsafe fn ptr_from_parts(ptr: *const (), _size: usize) -> *const Self {
        ptr as *const Self
    }

    unsafe fn ptr_from_parts_mut(ptr: *mut (), _size: usize) -> *mut Self {
        ptr as *mut Self
    }
}

unsafe impl<T: Sized> WrappableUnderlying for [T] {
    unsafe fn ptr_from_parts(ptr: *const (), size: usize) -> *const Self {
        // SAFETY: The caller is responsible for ensuring that `ptr` points to
        //         a `T` instance of length `size` bytes.
        unsafe { slice::from_raw_parts(ptr as *mut T, size / size_of::<T>()) }
    }

    unsafe fn ptr_from_parts_mut(ptr: *mut (), size: usize) -> *mut Self {
        // SAFETY: The caller is responsible for ensuring that `ptr` points to
        //         a `T` instance of length `size` bytes.
        unsafe { slice::from_raw_parts_mut(ptr as *mut T, size / size_of::<T>()) }
    }
}

pub trait Wrappable<'a>: ScriptBound + 'a {
    /// The type of data stored directly by the wrapper's data buffer.
    type InternalFormat: WrappableUnderlying + ?Sized;
    type Owned: Sized;

    fn write_into(self, dst: &mut ManuallyDrop<Self::InternalFormat>);

    fn get_required_buffer_size(&self) -> usize;

    /// Unwraps the value stored by the passed wrapper object.
    fn unwrap_as_value(wrapper: &WrappedObject) -> Result<Self::Owned, ()>;

    fn from_owned(owned: &'a Self::Owned) -> Self;
}

macro_rules! impl_wrappable {
    ($ty:ty, $enum_var:ident) => (
        impl $crate::ScriptBound for $ty {
            fn get_object_type() -> $crate::ObjectType {
                ObjectType {
                    ty: $crate::FundamentalType::$enum_var,
                    size: ::std::mem::size_of::<Self>(),
                    is_const: false,
                    is_refable: None,
                    is_refable_getter: None,
                    base_type_id: None,
                    base_type_name: None,
                    primary_type: None,
                    secondary_type: None,
                    synthetic_type: None,
                    callback_info: None,
                    copy_ctor: None,
                    dtor: None,
                }
            }
        }

        impl<'a> $crate::Wrappable<'a> for $ty {
            type InternalFormat = Self;
            type Owned = Self;

            fn write_into(self, dst: &mut ManuallyDrop<Self::InternalFormat>)
            where
                Self: $crate::Wrappable<'a, InternalFormat = Self> + ::std::clone::Clone {
                *dst = ManuallyDrop::new(self);
            }

            fn get_required_buffer_size(&self) -> usize
            where
                Self: $crate::Wrappable<'a, InternalFormat = Self> + ::std::clone::Clone {
                ::std::mem::size_of::<Self>()
            }

            fn unwrap_as_value(wrapper: &$crate::WrappedObject) -> Result<Self::Owned, ()>
            where
                Self: $crate::Wrappable<'a, InternalFormat = Self, Owned = Self> + ::std::clone::Clone {
                assert!(wrapper.populated);
                Ok(<Self::InternalFormat as Clone>::clone(wrapper.get::<Self>()?))
            }

            fn from_owned(owned: &'a Self::Owned) -> Self {
                *owned
            }
        }
    )
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
            ty: FundamentalType::String,
            size: 0,
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            base_type_id: None,
            base_type_name: None,
            primary_type: None,
            secondary_type: None,
            synthetic_type: None,
            callback_info: None,
            copy_ctor: Some(CopyCtorWrapper::of(copy_string)),
            dtor: Some(DtorWrapper::of(drop_string)),
        }
    }
}

impl<'a> Wrappable<'a> for String {
    type InternalFormat = [u8];
    type Owned = Self;

    fn write_into(self, dst: &mut ManuallyDrop<Self::InternalFormat>) {
        let cstr = CString::new(self.as_str()).unwrap();
        dst.copy_from_slice(cstr.as_bytes_with_nul());
    }

    fn get_required_buffer_size(&self) -> usize {
        let temp = CString::new(self.as_str()).unwrap().count_bytes();
        temp + 1
    }

    fn unwrap_as_value(wrapper: &WrappedObject) -> Result<Self::Owned, ()> {
        assert_eq!(wrapper.ty.ty, FundamentalType::String, "Wrong object type");
        assert!(wrapper.populated);

        let temp = wrapper.get::<Self>()?;
        let c_str = CStr::from_bytes_with_nul(temp)
            .expect("Malformed string in WrappedObject!");
        Ok(c_str.to_str().unwrap().to_string())
    }

    fn from_owned(owned: &'a Self::Owned) -> Self {
        owned.clone()
    }
}

impl ScriptBound for &str {
    fn get_object_type() -> ObjectType {
        ObjectType {
            ty: FundamentalType::String,
            size: 0,
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            base_type_id: None,
            base_type_name: None,
            primary_type: None,
            secondary_type: None,
            synthetic_type: None,
            callback_info: None,
            copy_ctor: Some(CopyCtorWrapper::of(copy_string)),
            dtor: None,
        }
    }
}

impl<'a> Wrappable<'a> for &'a str {
    type InternalFormat = [u8];
    type Owned = String;

    fn write_into(self, dst: &mut ManuallyDrop<Self::InternalFormat>) {
        let cstr = CString::new(self).unwrap();
        dst.copy_from_slice(cstr.as_bytes_with_nul());
    }

    fn get_required_buffer_size(&self) -> usize {
        let temp = CString::new(*self).unwrap().count_bytes();
        temp + 1
    }

    fn unwrap_as_value(wrapper: &WrappedObject) -> Result<Self::Owned, ()> {
        assert_eq!(wrapper.ty.ty, FundamentalType::String, "Wrong object type");
        assert!(wrapper.populated);

        let c_str = CStr::from_bytes_with_nul(
            &wrapper.get::<Self>()?[..wrapper.buffer_size]
        )
            .expect("Malformed string in WrappedObject");
        Ok(c_str.to_str().unwrap().to_string())
    }

    fn from_owned(owned: &'a Self::Owned) -> Self {
        owned.as_str()
    }
}

impl<T: ScriptBound> ScriptBound for Vec<T> {
    fn get_object_type() -> ObjectType {
        ObjectType {
            ty: FundamentalType::Vec,
            size: 0,
            is_const: true,
            is_refable: None,
            is_refable_getter: None,
            base_type_id: None,
            base_type_name: None,
            primary_type: Some(Box::new(T::get_object_type())),
            secondary_type: None,
            synthetic_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        }
    }
}

impl<'a, T: Wrappable<'a>> Wrappable<'a> for Vec<T> {
    type InternalFormat = i32;
    type Owned = Self;

    fn write_into(self, _dst: &mut ManuallyDrop<Self::InternalFormat>) {
        todo!()
    }

    fn get_required_buffer_size(&self) -> usize {
        todo!()
    }

    fn unwrap_as_value(_wrapper: &WrappedObject) -> Result<Self::Owned, ()> {
        todo!()
    }

    fn from_owned(_owned: &Self::Owned) -> Self {
        todo!()
    }
}

impl<T: ScriptBound> ScriptBound for &T {
    fn get_object_type() -> ObjectType {
        let base_type = T::get_object_type();
        ObjectType {
            ty: FundamentalType::Reference,
            size: size_of::<*const ()>(),
            is_const: false, //TODO: this totally breaks safety in bound functions
            is_refable: None,
            is_refable_getter: None,
            base_type_id: base_type.base_type_id,
            base_type_name: base_type.base_type_name,
            primary_type: Some(Box::new(T::get_object_type())),
            secondary_type: None,
            synthetic_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        }
    }
}

impl<'a, T: ScriptBound + 'a> Wrappable<'a> for &'a T {
    type InternalFormat = *const T;
    type Owned = Self;

    fn write_into(self, dst: &mut ManuallyDrop<Self::InternalFormat>) {
        *dst = ManuallyDrop::new(self);
    }

    fn get_required_buffer_size(&self) -> usize {
        size_of::<Self::InternalFormat>()
    }

    /// Unwraps the value stored by the passed wrapper object.
    ///
    /// # Safety
    /// The
    fn unwrap_as_value(wrapper: &WrappedObject) -> Result<Self, ()> {
        assert_eq!(wrapper.ty.ty, FundamentalType::Reference, "Wrong object type");

        unsafe { Ok(wrapper.get::<Self>()?.as_ref().unwrap()) }
    }

    fn from_owned(owned: &'a Self::Owned) -> Self {
        *owned
    }
}

impl<T: ScriptBound> ScriptBound for &mut T {
    fn get_object_type() -> ObjectType {
        let base_type = T::get_object_type();
        ObjectType {
            ty: FundamentalType::Reference,
            size: size_of::<*const ()>(),
            is_const: false,
            is_refable: None,
            is_refable_getter: None,
            base_type_id: base_type.base_type_id,
            base_type_name: base_type.base_type_name,
            primary_type: Some(Box::new(T::get_object_type())),
            secondary_type: None,
            synthetic_type: None,
            callback_info: None,
            copy_ctor: None,
            dtor: None,
        }
    }
}

impl<'a, T: ScriptBound + 'a> Wrappable<'a> for &'a mut T {
    type InternalFormat = *mut T;
    type Owned = &'a mut T;

    fn write_into(self, dst: &mut ManuallyDrop<Self::InternalFormat>) {
        *dst = ManuallyDrop::new(self)
    }

    fn get_required_buffer_size(&self) -> usize {
        size_of::<Self::InternalFormat>()
    }

    fn unwrap_as_value(wrapper: &WrappedObject) -> Result<Self::Owned, ()> {
        assert_eq!(wrapper.ty.ty, FundamentalType::Reference, "Wrong object type");
        assert!(!wrapper.ty.is_const);
        assert!(wrapper.populated);

        unsafe { Ok(wrapper.get::<Self>()?.as_mut().unwrap()) }
    }

    fn from_owned(owned: &'a Self::Owned) -> Self {
        unsafe { *ptr::from_ref(owned).cast_mut() }
    }
}

const WRAPPED_OBJ_BUF_CAP: usize = 64;

pub type EnumValueGetter = fn() -> i64;

pub struct WrappedObject {
    ty: ObjectType,
    frontend_type_id: RefCell<Option<String>>,
    synthetic_type_name: Option<String>,
    data: ManuallyDrop<[u8; WRAPPED_OBJ_BUF_CAP]>,
    is_on_heap: bool,
    buffer_size: usize,
    populated: bool,
    drop_fn: fn(&mut WrappedObject),
}

impl Drop for WrappedObject {
    fn drop(&mut self) {
        (self.drop_fn)(self);
        if self.is_on_heap {
            unsafe {
                dealloc(
                    self.get_raw_heap_ptr_mut().cast(),
                    Layout::from_size_align(self.buffer_size, 8).unwrap()
                );
            }
        }
    }
}

impl WrappedObject {
    /// Returns a representation of the type of object or primitive stored by
    /// this wrapper.
    pub fn get_type(&self) -> &ObjectType {
        &self.ty
    }

    /// Returns the size in bytes of the underlying storage for the wrapped
    /// object.
    pub fn get_buffer_size(&self) -> usize {
        self.buffer_size
    }

    /// Returns whether this wrapper is currently populated.
    pub fn is_populated(&self) -> bool {
        self.populated
    }

    /// Creates a new [ObjectWrapper] and stores the given parameter into it as
    /// an initial value.
    pub fn wrap<'a, T: Wrappable<'a>>(val: T) -> Self {
        let size = val.get_required_buffer_size();

        let mut wrapper = Self::new(T::get_object_type(), size);
        wrapper.drop_fn = |wrapper: &mut WrappedObject| {
            unsafe { ManuallyDrop::drop(wrapper.get_mut::<T>().unwrap()); }
        };

        if wrapper.frontend_type_id.borrow().is_none() {
            wrapper.frontend_type_id.replace(Some(format!("{:?}", typeid::of::<T>())));
        }

        // SAFETY: We don't read from the returned reference
        val.write_into(unsafe { wrapper.get_mut_maybe_uninit::<T>().unwrap() });

        wrapper.populated = true;

        wrapper
    }

    /// Creates a new unpopulated [ObjectWrapper].
    pub fn new(ty: ObjectType, size: usize) -> Self {
        let synthetic_type_name = ty.synthetic_type.as_ref().cloned();
        let mut wrapper = WrappedObject {
            ty,
            frontend_type_id: RefCell::new(None),
            synthetic_type_name,
            data: ManuallyDrop::new([0; WRAPPED_OBJ_BUF_CAP]),
            is_on_heap: false,
            buffer_size: size,
            populated: false,
            drop_fn: |_| {},
        };

        if size > WRAPPED_OBJ_BUF_CAP {
            let heap_ptr: *mut u8 =
                unsafe {
                    alloc_zeroed(Layout::from_size_align(wrapper.buffer_size, 8).unwrap()).cast()
                };
            unsafe {
                // copy pointer address to internal buffer
                *wrapper.data.as_mut_ptr().cast() = heap_ptr;
            }
            wrapper.is_on_heap = true;
        } else {
            wrapper.is_on_heap = false;
        }

        if wrapper.ty.ty == FundamentalType::Empty {
            wrapper.populated = true;
            assert!(wrapper.frontend_type_id.borrow().is_none());
            wrapper.frontend_type_id.replace(Some(format!("{:?}", typeid::of::<()>())));
        }

        wrapper
    }

    /// Unwraps the value stored in this [ObjectWrapper] and returns it as a
    /// caller-owned representation. See [ObjectWrapper::Owned] for more
    /// information.
    ///
    /// # Safety
    /// - The passed type parameter must be consistent across all calls to this
    ///   object's functions which accept a type parameter. Making any sequence
    ///   of calls against the same [WrappedObject] across its lifetime with
    ///   different type parameters is UB.
    pub fn unwrap<'a, T: Wrappable<'a>>(&self) -> Result<T::Owned, ()> {
        if !self.populated {
            return Err(());
        }

        self.validate_type::<T>()?;

        <T as Wrappable<'a>>::unwrap_as_value(self)
    }

    /// Returns a reference to the wrapper's underlying buffer, or [None] if it
    /// is not currently populated.
    ///
    /// # Safety
    /// - The passed type parameter must be consistent across all calls to this
    ///   object's functions which accept a type parameter. Making any sequence
    ///   of calls against the same [WrappedObject] across its lifetime with
    ///   different type parameters is UB.
    pub fn get<'a, T: Wrappable<'a>>(&self) -> Result<&T::InternalFormat, ()> {
        if !self.populated {
            return Err(());
        }

        self.validate_type::<T>()?;

        // SAFETY: The caller is responsible for ensuring that the same type
        //         parameter is used across all calls, ensuring that the pointer
        //         constructed here is consistent with the data stored in the
        //         buffer.
        unsafe {
            Ok(&*T::InternalFormat::ptr_from_parts(self.get_raw_ptr().cast(), self.buffer_size))
        }
    }

    /// Returns a mutable reference to the wrapper's underlying buffer, or
    /// [None] if it is not currently populated.
    ///
    /// # Safety
    /// - The passed type parameter must be consistent across all calls to this
    ///   object's functions which accept a type parameter. Making any sequence
    ///   of calls against the same [WrappedObject] across its lifetime with
    ///   different type parameters is UB.
    pub fn get_mut<'a, T: Wrappable<'a>>(&mut self)
        -> Result<&mut ManuallyDrop<T::InternalFormat>, ()> {
        if !self.populated {
            return Err(());
        }

        self.validate_type::<T>()?;

        // SAFETY: We just verified that the wrapper is populated
        unsafe { self.get_mut_maybe_uninit::<T>() }
    }

    /// Returns a mutable reference to the wrapper's underlying buffer.
    ///
    /// # Safety
    /// - The passed type parameter must be consistent across all calls to this
    ///   object's functions which accept a type parameter. Making any sequence
    ///   of calls against the same [WrappedObject] across its lifetime with
    ///   different type parameters is UB.
    /// - The returned reference must not be read from if
    ///   [WrappedObject::is_populated] returns `false`. Doing so is UB.
    pub unsafe fn get_mut_maybe_uninit<'a, T: Wrappable<'a>>(&mut self)
        -> Result<&mut ManuallyDrop<T::InternalFormat>, ()> {
        self.validate_type::<T>()?;

        let p = self.get_raw_mut_ptr().cast();
        // SAFETY: The caller is responsible for ensuring that the same type
        //         parameter is used across all calls, ensuring that the pointer
        //         constructed here is consistent with the data stored in the
        //         buffer.
        unsafe {
            Ok((T::InternalFormat::ptr_from_parts_mut(p, self.buffer_size)
                as *mut ManuallyDrop<T::InternalFormat>)
                .as_mut().unwrap())
        }
    }

    /// Returns a raw pointer to the wrapper's underlying buffer.
    ///
    /// Note that while calling this function is safe, dereferencing the
    /// returned pointer is not and the caller is responsible for ensuring that
    /// it is used in a consistent manner with respect to typing.
    pub fn get_raw_ptr(&self) -> *const ManuallyDrop<()> {
        if self.is_on_heap {
            self.get_raw_heap_ptr().cast()
        } else {
            self.data.as_ptr().cast()
        }
    }

    /// Returns a mutable raw pointer to the wrapper's underlying buffer.
    ///
    /// Note that while calling this function is safe, dereferencing the
    /// returned pointer is not and the caller is responsible for ensuring that
    /// it is used in a consistent manner with respect to typing.
    pub fn get_raw_mut_ptr(&mut self) -> *mut ManuallyDrop<()> {
        if self.is_on_heap {
            self.get_raw_heap_ptr_mut().cast()
        } else {
            self.data.as_mut_ptr().cast()
        }
    }

    /// Stores a value into the wrapper, dropping the old value if applicable..
    ///
    /// # Safety
    /// - The passed type parameter must be consistent across all calls to this
    ///   object which accept a type parameter. Making any sequence of calls to
    ///   the same [WrappedObject] across its lifetime with different type
    ///   parameters is UB.
    pub fn store_value<'a, T: Wrappable<'a>>(&mut self, val: T) -> Result<(), ()> {
        self.validate_type::<T>()?;

        if self.populated {
            (self.drop_fn)(self)
        }

        // SAFETY: We don't read from the returned pointer
        val.write_into(unsafe { self.get_mut_maybe_uninit::<T>().unwrap() });
        self.populated = true;

        Ok(())
    }

    /// Stores a value directly into the wrapper's underlying buffer. This
    /// skips conversion from the wrapper's nominal type into its internal
    /// format.
    ///
    /// # Safety
    /// The passed object is consistent with other calls against this object to
    /// functions which accept a type parameter.
    pub unsafe fn store_raw<T>(&mut self, val: T) -> Result<(), ()> {
        //assert!(self.frontend_type_id.is_some());

        if size_of::<T>() > self.buffer_size {
            return Err(());
        }

        if self.populated {
            (self.drop_fn)(self);
        }

        // SAFETY: `get_mut` is guaranteed to return a valid pointer to at
        //         least `buffer_size` bytes of memory.
        unsafe { *self.get_raw_mut_ptr().cast() = ManuallyDrop::new(val); }
        self.drop_fn = |_wrapper: &mut WrappedObject| {
            //TODO
        };
        self.populated = true;

        Ok(())
    }

    /// Copies the contents of the wrapper's underlying buffer directly into the
    /// provided slice.
    ///
    /// Note that while calling this function is safe, any further operations on
    /// on the passed slice must be consistent with the type of data stored by
    /// the wrapper.
    pub fn copy_to_slice(&self, dst: &mut [u8]) {
        assert!(dst.len() <= self.buffer_size);

        let src = unsafe {
            &*slice_from_raw_parts(self.get_raw_ptr().cast(), self.buffer_size)
        };
        dst.copy_from_slice(src);
    }

    /// Copies the contents of the provided slice directly into the wrapper's
    /// underlying buffer.
    ///
    /// # Safety
    /// The passed slice must be consistent with other calls to this object
    /// which accept a type parameter. Specifically, the copied bytes must
    /// correspond to the internal format of the type being wrapped.
    pub unsafe fn copy_from_slice(&mut self, src: &[u8]) {
        assert!(src.len() <= self.buffer_size);

        // SAFETY: The caller is responsible for ensuring that the slice
        //         contents are consistent with the type stored by the wrapper.
        let dst = unsafe {
            &mut *slice_from_raw_parts_mut(self.get_raw_mut_ptr().cast(), self.buffer_size)
        };
        dst.copy_from_slice(src);

        self.populated = true;
    }

    fn get_raw_heap_ptr(&self) -> *const ManuallyDrop<[u8]> {
        assert!(self.is_on_heap);
        // SAFETY: The contents of `data` are guaranteed to be a pointer to
        //         valid heap memory when `is_on_heap` is true.
        unsafe { *self.data.as_ptr().cast() }
    }

    fn get_raw_heap_ptr_mut(&mut self) -> *mut ManuallyDrop<[u8]> {
        assert!(self.is_on_heap);
        // SAFETY: The contents of `data` are guaranteed to be a pointer to
        //         valid heap memory when `is_on_heap` is true.
        unsafe { *self.data.as_ptr().cast() }
    }

    fn validate_type<'a, T: Wrappable<'a>>(&self) -> Result<(), ()> {
        let other_ty = T::get_object_type();

        // Special cases where the TypeId shortcut doesn't work and a simple
        // type-specific check suffices instead.

        // Enum values can be stored/accessed as their underlying integral
        // type. Integral types are essentially identical except for their size, so
        // This does technically leave a gap in detecting signed vs.
        // unsigned integer types, but that should be fairly benign in
        // practice.
        if self.ty.ty.is_integral() && other_ty.ty.is_integral() {
            return if self.ty.size == other_ty.size {
                Ok(())
            } else {
                Err(())
            };
        }

        // Strings are slightly problematic because of String vs &str. As long
        // as both are string types it should be safe to proceed.
        if self.ty.ty == FundamentalType::String && other_ty.ty == FundamentalType::String {
            return Ok(());
        }

        // Once we've already validated a particular type parameter for this
        // object, we can just check against that to ensure that all subsequent
        // accesses to the object are consistent. This saves us from having to
        // do a relatively expensive manual comparison each time.
        if self.frontend_type_id.borrow().is_some() {
            return if self.frontend_type_id.borrow().as_ref().unwrap().as_str() ==
                format!("{:?}", typeid::of::<T>()) {
                Ok(())
            } else {
                Err(())
            }
        }

        compare_types(&self.ty, &other_ty)?;

        if self.ty.ty == FundamentalType::Callback {
            if let Some(synthetic_type_name) = self.synthetic_type_name.as_ref() &&
                synthetic_type_name.as_str() != type_name::<T>().split("::").last().unwrap()
            {
                return Err(())
            }
        }

        self.frontend_type_id.replace(Some(format!("{:?}", typeid::of::<T>())));

        Ok(())
    }
}

fn compare_types(a: &ObjectType, b: &ObjectType) -> Result<(), ()> {
    if a.size != b.size {
        return Err(());
    }

    if a.ty != b.ty &&
        !((a.ty.is_integral() && b.ty == FundamentalType::Enum) ||
            (b.ty.is_integral() && a.ty == FundamentalType::Enum)) {
        return Err(())
    }

    if a.primary_type.is_some() != b.primary_type.is_some() {
        return Err(())
    }
    if a.primary_type.is_some() {
        compare_types(a.primary_type.as_ref().unwrap(), b.primary_type.as_ref().unwrap())?;
    }

    if a.secondary_type.is_some() != b.secondary_type.is_some() {
        return Err(());
    }
    if a.secondary_type.is_some() {
        compare_types(a.secondary_type.as_ref().unwrap(), b.secondary_type.as_ref().unwrap())?;
    }

    if a.ty == FundamentalType::Object {
        if a.base_type_id.is_none() || b.base_type_id.is_none() ||
            a.base_type_id.as_ref().unwrap() != b.base_type_id.as_ref().unwrap() {
            return Err(());
        }
    }

    Ok(())
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
    pub values: &'static [(&'static str, EnumValueGetter)],
}

pub unsafe extern "C" fn copy_string(dst: *mut (), src: *const ()) {
    unsafe {
        let len = CStr::from_ptr(src.cast()).count_bytes() + 1;
        let src_slice = slice::from_raw_parts(src, len);
        let dst_slice = slice::from_raw_parts_mut(dst, len);
        dst_slice.copy_from_slice(src_slice);
    }
}

pub unsafe extern "C" fn move_string(dst: *mut (), src: *mut ()) {
    unsafe { copy_string(dst, src); }
}

pub unsafe extern "C" fn drop_string(target: *mut ()) {
    unsafe { ptr::drop_in_place(target.cast::<String>()); }
}

/*impl WrappedScriptCallback {
    pub unsafe fn call(
        callback: WrappedScriptCallback,
        params: Vec<WrappedObject>,
    ) -> Result<WrappedObject, ScriptInvocationError> {
        (callback.entry_point)(
            params,
            callback.userdata,
        )
    }
}*/

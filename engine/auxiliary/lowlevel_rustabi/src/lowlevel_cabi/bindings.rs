/* automatically generated by rust-bindgen 0.69.4 */

#![allow(non_camel_case_types, unused_imports, unused_qualifications)]
use super::*;

pub type StringArray = *mut ::std::os::raw::c_void;
pub type StringArrayConst = *const ::std::os::raw::c_void;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ArgusHandle {
    pub index: u32,
    pub uid: u32,
}
pub type argus_handle_table_t = *mut ::std::os::raw::c_void;
pub type argus_handle_table_const_t = *const ::std::os::raw::c_void;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct argus_vector_2d_t {
    pub x: f64,
    pub y: f64,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct argus_vector_3d_t {
    pub __bindgen_anon_1: argus_vector_3d_t__bindgen_ty_1,
    pub __bindgen_anon_2: argus_vector_3d_t__bindgen_ty_2,
    pub __bindgen_anon_3: argus_vector_3d_t__bindgen_ty_3,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3d_t__bindgen_ty_1 {
    pub x: f64,
    pub r: f64,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3d_t__bindgen_ty_2 {
    pub y: f64,
    pub g: f64,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3d_t__bindgen_ty_3 {
    pub z: f64,
    pub b: f64,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct argus_vector_4d_t {
    pub __bindgen_anon_1: argus_vector_4d_t__bindgen_ty_1,
    pub __bindgen_anon_2: argus_vector_4d_t__bindgen_ty_2,
    pub __bindgen_anon_3: argus_vector_4d_t__bindgen_ty_3,
    pub __bindgen_anon_4: argus_vector_4d_t__bindgen_ty_4,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4d_t__bindgen_ty_1 {
    pub x: f64,
    pub r: f64,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4d_t__bindgen_ty_2 {
    pub y: f64,
    pub g: f64,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4d_t__bindgen_ty_3 {
    pub z: f64,
    pub b: f64,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4d_t__bindgen_ty_4 {
    pub w: f64,
    pub a: f64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct argus_vector_2f_t {
    pub x: f32,
    pub y: f32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct argus_vector_3f_t {
    pub __bindgen_anon_1: argus_vector_3f_t__bindgen_ty_1,
    pub __bindgen_anon_2: argus_vector_3f_t__bindgen_ty_2,
    pub __bindgen_anon_3: argus_vector_3f_t__bindgen_ty_3,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3f_t__bindgen_ty_1 {
    pub x: f32,
    pub r: f32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3f_t__bindgen_ty_2 {
    pub y: f32,
    pub g: f32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3f_t__bindgen_ty_3 {
    pub z: f32,
    pub b: f32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct argus_vector_4f_t {
    pub __bindgen_anon_1: argus_vector_4f_t__bindgen_ty_1,
    pub __bindgen_anon_2: argus_vector_4f_t__bindgen_ty_2,
    pub __bindgen_anon_3: argus_vector_4f_t__bindgen_ty_3,
    pub __bindgen_anon_4: argus_vector_4f_t__bindgen_ty_4,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4f_t__bindgen_ty_1 {
    pub x: f32,
    pub r: f32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4f_t__bindgen_ty_2 {
    pub y: f32,
    pub g: f32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4f_t__bindgen_ty_3 {
    pub z: f32,
    pub b: f32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4f_t__bindgen_ty_4 {
    pub w: f32,
    pub a: f32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct argus_vector_2i_t {
    pub x: i32,
    pub y: i32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct argus_vector_3i_t {
    pub __bindgen_anon_1: argus_vector_3i_t__bindgen_ty_1,
    pub __bindgen_anon_2: argus_vector_3i_t__bindgen_ty_2,
    pub __bindgen_anon_3: argus_vector_3i_t__bindgen_ty_3,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3i_t__bindgen_ty_1 {
    pub x: i32,
    pub r: i32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3i_t__bindgen_ty_2 {
    pub y: i32,
    pub g: i32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3i_t__bindgen_ty_3 {
    pub z: i32,
    pub b: i32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct argus_vector_4i_t {
    pub __bindgen_anon_1: argus_vector_4i_t__bindgen_ty_1,
    pub __bindgen_anon_2: argus_vector_4i_t__bindgen_ty_2,
    pub __bindgen_anon_3: argus_vector_4i_t__bindgen_ty_3,
    pub __bindgen_anon_4: argus_vector_4i_t__bindgen_ty_4,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4i_t__bindgen_ty_1 {
    pub x: i32,
    pub r: i32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4i_t__bindgen_ty_2 {
    pub y: i32,
    pub g: i32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4i_t__bindgen_ty_3 {
    pub z: i32,
    pub b: i32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4i_t__bindgen_ty_4 {
    pub w: i32,
    pub a: i32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct argus_vector_2u_t {
    pub x: u32,
    pub y: u32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct argus_vector_3u_t {
    pub __bindgen_anon_1: argus_vector_3u_t__bindgen_ty_1,
    pub __bindgen_anon_2: argus_vector_3u_t__bindgen_ty_2,
    pub __bindgen_anon_3: argus_vector_3u_t__bindgen_ty_3,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3u_t__bindgen_ty_1 {
    pub x: u32,
    pub r: u32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3u_t__bindgen_ty_2 {
    pub y: u32,
    pub g: u32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_3u_t__bindgen_ty_3 {
    pub z: u32,
    pub b: u32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct argus_vector_4u_t {
    pub __bindgen_anon_1: argus_vector_4u_t__bindgen_ty_1,
    pub __bindgen_anon_2: argus_vector_4u_t__bindgen_ty_2,
    pub __bindgen_anon_3: argus_vector_4u_t__bindgen_ty_3,
    pub __bindgen_anon_4: argus_vector_4u_t__bindgen_ty_4,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4u_t__bindgen_ty_1 {
    pub x: u32,
    pub r: u32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4u_t__bindgen_ty_2 {
    pub y: u32,
    pub g: u32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4u_t__bindgen_ty_3 {
    pub z: u32,
    pub b: u32,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union argus_vector_4u_t__bindgen_ty_4 {
    pub w: u32,
    pub a: u32,
}
pub type message_dispatcher_t = ::std::option::Option<
    unsafe extern "C" fn(arg1: *const ::std::os::raw::c_char, arg2: *const ::std::os::raw::c_void),
>;
extern "C" {
    pub fn string_array_get_count(sa: StringArrayConst) -> usize;
    pub fn string_array_get_element(
        sa: StringArrayConst,
        index: usize,
    ) -> *const ::std::os::raw::c_char;
    pub fn string_array_free(sa: StringArray);
    pub fn argus_handle_table_new() -> argus_handle_table_t;
    pub fn argus_handle_table_delete(table: argus_handle_table_t);
    pub fn argus_handle_table_create_handle(
        table: argus_handle_table_t,
        ptr: *mut ::std::os::raw::c_void,
    ) -> ArgusHandle;
    pub fn argus_handle_table_copy_handle(
        table: argus_handle_table_t,
        handle: ArgusHandle,
    ) -> ArgusHandle;
    pub fn argus_handle_table_update_handle(
        table: argus_handle_table_t,
        handle: ArgusHandle,
        ptr: *mut ::std::os::raw::c_void,
    ) -> bool;
    pub fn argus_handle_table_release_handle(table: argus_handle_table_t, handle: ArgusHandle);
    pub fn argus_handle_table_deref(
        table: argus_handle_table_const_t,
        handle: ArgusHandle,
    ) -> *mut ::std::os::raw::c_void;
    pub fn argus_set_message_dispatcher(dispatcher: message_dispatcher_t);
    pub fn argus_broadcast_message(
        type_id: *const ::std::os::raw::c_char,
        message: *const ::std::os::raw::c_void,
    );
}

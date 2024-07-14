/* automatically generated by rust-bindgen 0.69.4 */

use super::*;

pub type argus_resource_t = *mut ::std::os::raw::c_void;
pub type argus_resource_const_t = *const ::std::os::raw::c_void;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct argus_resource_prototype_t {
    pub uid: *const ::std::os::raw::c_char,
    pub media_type: *const ::std::os::raw::c_char,
    pub fs_path: *const ::std::os::raw::c_char,
}
pub const RESOURCE_ERROR_REASON_GENERIC: ResourceErrorReason = 0;
pub const RESOURCE_ERROR_REASON_NOT_FOUND: ResourceErrorReason = 1;
pub const RESOURCE_ERROR_REASON_NOT_LOADED: ResourceErrorReason = 2;
pub const RESOURCE_ERROR_REASON_ALREADY_LOADED: ResourceErrorReason = 3;
pub const RESOURCE_ERROR_REASON_NO_LOADER: ResourceErrorReason = 4;
pub const RESOURCE_ERROR_REASON_LOAD_FAILED: ResourceErrorReason = 5;
pub const RESOURCE_ERROR_REASON_MALFORMED_CONTENT: ResourceErrorReason = 6;
pub const RESOURCE_ERROR_REASON_INVALID_CONTENT: ResourceErrorReason = 7;
pub const RESOURCE_ERROR_REASON_UNSUPPORTED_CONTENT: ResourceErrorReason = 8;
pub const RESOURCE_ERROR_REASON_UNEXPECTED_REFERENCE_TYPE: ResourceErrorReason = 9;
pub type ResourceErrorReason = ::std::os::raw::c_uint;
pub type argus_resource_error_t = *mut ::std::os::raw::c_void;
pub const k_event_type_resource: &[u8; 9] = b"resource\0";
pub const RESOURCE_EVENT_TYPE_LOAD: ResourceEventType = 0;
pub const RESOURCE_EVENT_TYPE_UNLOAD: ResourceEventType = 1;
pub type ResourceEventType = ::std::os::raw::c_uint;
pub type argus_resource_event_t = *mut ::std::os::raw::c_void;
pub type argus_resource_event_const_t = *const ::std::os::raw::c_void;
pub type argus_resource_manager_t = *mut ::std::os::raw::c_void;
pub type argus_resource_manager_const_t = *const ::std::os::raw::c_void;
pub type argus_resource_loader_t = *mut ::std::os::raw::c_void;
#[repr(C)]
#[derive(Copy, Clone)]
pub struct ResourceOrResourceError {
    pub is_ok: bool,
    pub ve: ResourceOrResourceError__bindgen_ty_1,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union ResourceOrResourceError__bindgen_ty_1 {
    pub value: argus_resource_t,
    pub error: argus_resource_error_t,
}
pub type argus_resource_loader_const_t = *const ::std::os::raw::c_void;
#[repr(C)]
#[derive(Copy, Clone)]
pub struct VoidPtrOrResourceError {
    pub is_ok: bool,
    pub ve: VoidPtrOrResourceError__bindgen_ty_1,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union VoidPtrOrResourceError__bindgen_ty_1 {
    pub value: *mut ::std::os::raw::c_void,
    pub error: argus_resource_error_t,
}
pub type argus_loaded_dependency_set_t = *mut ::std::os::raw::c_void;
#[repr(C)]
#[derive(Copy, Clone)]
pub struct LoadedDependencySetOrResourceError {
    pub is_ok: bool,
    pub ve: LoadedDependencySetOrResourceError__bindgen_ty_1,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union LoadedDependencySetOrResourceError__bindgen_ty_1 {
    pub value: argus_loaded_dependency_set_t,
    pub error: argus_resource_error_t,
}
pub type argus_resource_read_callback_t = ::std::option::Option<
    unsafe extern "C" fn(
        dst: *mut ::std::os::raw::c_void,
        len: usize,
        data: *mut ::std::os::raw::c_void,
    ) -> bool,
>;
pub type argus_resource_load_fn_t = ::std::option::Option<
    unsafe extern "C" fn(
        loader: argus_resource_loader_t,
        manager: argus_resource_manager_t,
        proto: argus_resource_prototype_t,
        read_callback: argus_resource_read_callback_t,
        size: usize,
        user_data: *mut ::std::os::raw::c_void,
        engine_data: *mut ::std::os::raw::c_void,
    ) -> VoidPtrOrResourceError,
>;
pub type argus_resource_copy_fn_t = ::std::option::Option<
    unsafe extern "C" fn(
        loader: argus_resource_loader_t,
        manager: argus_resource_manager_t,
        proto: argus_resource_prototype_t,
        src: *mut ::std::os::raw::c_void,
        data: *mut ::std::os::raw::c_void,
    ) -> VoidPtrOrResourceError,
>;
pub type argus_resource_unload_fn_t = ::std::option::Option<
    unsafe extern "C" fn(
        loader: argus_resource_loader_t,
        ptr: *mut ::std::os::raw::c_void,
        user_data: *mut ::std::os::raw::c_void,
    ),
>;
extern "C" {
    pub fn argus_resource_get_prototype(
        resource: argus_resource_const_t,
    ) -> argus_resource_prototype_t;
    pub fn argus_resource_release(resource: argus_resource_const_t);
    pub fn argus_resource_get_data_ptr(
        resource: argus_resource_const_t,
    ) -> *const ::std::os::raw::c_void;
    pub fn argus_resource_error_new(
        reason: ResourceErrorReason,
        uid: *const ::std::os::raw::c_char,
        info: *const ::std::os::raw::c_char,
    ) -> argus_resource_error_t;
    pub fn argus_resource_error_destruct(error: argus_resource_error_t);
    pub fn argus_resource_error_get_reason(error: argus_resource_error_t) -> ResourceErrorReason;
    pub fn argus_resource_error_get_uid(
        error: argus_resource_error_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_resource_error_get_info(
        error: argus_resource_error_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_resource_event_get_subtype(
        event: argus_resource_event_const_t,
    ) -> ResourceEventType;
    pub fn argus_resource_event_get_prototype(
        event: argus_resource_event_const_t,
    ) -> argus_resource_prototype_t;
    pub fn argus_resource_event_get_resource(event: argus_resource_event_t) -> argus_resource_t;
    pub fn argus_resource_manager_get_instance() -> argus_resource_manager_t;
    pub fn argus_resource_manager_discover_resources(mgr: argus_resource_manager_t);
    pub fn argus_resource_manager_add_memory_package(
        mgr: argus_resource_manager_t,
        buf: *const ::std::os::raw::c_uchar,
        len: usize,
    );
    pub fn argus_resource_manager_register_loader(
        mgr: argus_resource_manager_t,
        media_types: *const *const ::std::os::raw::c_char,
        media_types_count: usize,
        loader: argus_resource_loader_t,
    );
    pub fn argus_resource_manager_get_resource(
        mgr: argus_resource_manager_t,
        uid: *const ::std::os::raw::c_char,
    ) -> ResourceOrResourceError;
    pub fn argus_resource_manager_get_resource_weak(
        mgr: argus_resource_manager_t,
        uid: *const ::std::os::raw::c_char,
    ) -> ResourceOrResourceError;
    pub fn argus_resource_manager_try_get_resource(
        mgr: argus_resource_manager_t,
        uid: *const ::std::os::raw::c_char,
    ) -> ResourceOrResourceError;
    pub fn argus_resource_manager_get_resource_async(
        mgr: argus_resource_manager_t,
        uid: *const ::std::os::raw::c_char,
        callback: ::std::option::Option<unsafe extern "C" fn(arg1: ResourceOrResourceError)>,
    );
    pub fn argus_resource_manager_create_resource(
        mgr: argus_resource_manager_t,
        uid: *const ::std::os::raw::c_char,
        media_type: *const ::std::os::raw::c_char,
        data: *const ::std::os::raw::c_void,
        len: usize,
    ) -> ResourceOrResourceError;
    pub fn argus_resource_loader_new(
        media_types: *const *const ::std::os::raw::c_char,
        media_types_count: usize,
        load_fn: argus_resource_load_fn_t,
        copy_fn: argus_resource_copy_fn_t,
        unload_fn: argus_resource_unload_fn_t,
        user_data: *mut ::std::os::raw::c_void,
    ) -> argus_resource_loader_t;
    pub fn argus_resource_loader_load_dependencies(
        loader: argus_resource_loader_t,
        manager: argus_resource_manager_t,
        dependencies: *const *const ::std::os::raw::c_char,
        dependencies_count: usize,
    ) -> LoadedDependencySetOrResourceError;
    pub fn argus_loaded_dependency_set_get_count(set: argus_loaded_dependency_set_t) -> usize;
    pub fn argus_loaded_dependency_set_get_name_at(
        set: argus_loaded_dependency_set_t,
        index: usize,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_loaded_dependency_set_get_resource_at(
        set: argus_loaded_dependency_set_t,
        index: usize,
    ) -> argus_resource_const_t;
    pub fn argus_loaded_dependency_set_destruct(set: argus_loaded_dependency_set_t);
}

/* automatically generated by rust-bindgen 0.69.4 */

#![allow(non_camel_case_types, unused_imports, unused_qualifications)]
use super::*;

pub type argus_binding_error_t = *mut ::std::os::raw::c_void;
pub type argus_binding_error_const_t = *const ::std::os::raw::c_void;
pub const BINDING_ERROR_TYPE_DUPLICATE_NAME: ArgusBindingErrorType = 0;
pub const BINDING_ERROR_TYPE_CONFLICTING_NAME: ArgusBindingErrorType = 1;
pub const BINDING_ERROR_TYPE_INVALID_DEFINITION: ArgusBindingErrorType = 2;
pub const BINDING_ERROR_TYPE_INVALID_MEMBERS: ArgusBindingErrorType = 3;
pub const BINDING_ERROR_TYPE_UNKNOWN_PARENT: ArgusBindingErrorType = 4;
pub const BINDING_ERROR_TYPE_OTHER: ArgusBindingErrorType = 5;
pub type ArgusBindingErrorType = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ArgusMaybeBindingError {
    pub is_err: bool,
    pub error: argus_binding_error_t,
}
pub type argus_reflective_args_error_t = *mut ::std::os::raw::c_void;
pub type argus_reflective_args_error_const_t = *const ::std::os::raw::c_void;
pub type argus_script_invocation_error_t = *mut ::std::os::raw::c_void;
pub type argus_script_invocation_error_const_t = *const ::std::os::raw::c_void;
pub type ArgusCopyCtorProxy = ::std::option::Option<
    unsafe extern "C" fn(arg1: *mut ::std::os::raw::c_void, arg2: *const ::std::os::raw::c_void),
>;
pub type ArgusMoveCtorProxy = ::std::option::Option<
    unsafe extern "C" fn(arg1: *mut ::std::os::raw::c_void, arg2: *mut ::std::os::raw::c_void),
>;
pub type ArgusDtorProxy =
    ::std::option::Option<unsafe extern "C" fn(arg1: *mut ::std::os::raw::c_void)>;
pub const INTEGRAL_TYPE_VOID: ArgusIntegralType = 0;
pub const INTEGRAL_TYPE_INTEGER: ArgusIntegralType = 1;
pub const INTEGRAL_TYPE_UINTEGER: ArgusIntegralType = 2;
pub const INTEGRAL_TYPE_FLOAT: ArgusIntegralType = 3;
pub const INTEGRAL_TYPE_BOOLEAN: ArgusIntegralType = 4;
pub const INTEGRAL_TYPE_STRING: ArgusIntegralType = 5;
pub const INTEGRAL_TYPE_STRUCT: ArgusIntegralType = 6;
pub const INTEGRAL_TYPE_POINTER: ArgusIntegralType = 7;
pub const INTEGRAL_TYPE_ENUM: ArgusIntegralType = 8;
pub const INTEGRAL_TYPE_CALLBACK: ArgusIntegralType = 9;
pub const INTEGRAL_TYPE_TYPE: ArgusIntegralType = 10;
pub const INTEGRAL_TYPE_VECTOR: ArgusIntegralType = 11;
pub const INTEGRAL_TYPE_VECTORREF: ArgusIntegralType = 12;
pub const INTEGRAL_TYPE_RESULT: ArgusIntegralType = 13;
pub type ArgusIntegralType = ::std::os::raw::c_uint;
pub const FUNCTION_TYPE_GLOBAL: ArgusFunctionType = 0;
pub const FUNCTION_TYPE_MEMBER_STATIC: ArgusFunctionType = 1;
pub const FUNCTION_TYPE_MEMBER_INSTANCE: ArgusFunctionType = 2;
pub const FUNCTION_TYPE_EXTENSION: ArgusFunctionType = 3;
pub type ArgusFunctionType = ::std::os::raw::c_uint;
pub const SYMBOL_TYPE_TYPE: ArgusSymbolType = 0;
pub const SYMBOL_TYPE_FIELD: ArgusSymbolType = 1;
pub const SYMBOL_TYPE_FUNCTION: ArgusSymbolType = 2;
pub type ArgusSymbolType = ::std::os::raw::c_uint;
pub type argus_object_type_t = *mut ::std::os::raw::c_void;
pub type argus_object_type_const_t = *const ::std::os::raw::c_void;
pub type argus_script_callback_type_t = *mut ::std::os::raw::c_void;
pub type argus_script_callback_type_const_t = *const ::std::os::raw::c_void;
pub type argus_object_wrapper_t = *mut ::std::os::raw::c_void;
pub type argus_object_wrapper_const_t = *const ::std::os::raw::c_void;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ArgusScriptCallbackType {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ArgusObjectWrapperOrReflectiveArgsError {
    pub is_err: bool,
    pub val: argus_object_wrapper_t,
    pub err: argus_reflective_args_error_t,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ArgusObjectWrapperOrScriptInvocationError {
    pub is_err: bool,
    pub val: argus_object_wrapper_t,
    pub err: argus_script_invocation_error_t,
}
pub type argus_script_callback_result_t = *mut ::std::os::raw::c_void;
pub type argus_script_callback_result_const_t = *const ::std::os::raw::c_void;
pub type ArgusProxiedNativeFunction = ::std::option::Option<
    unsafe extern "C" fn(
        params_count: usize,
        params: *const argus_object_wrapper_t,
        extra: *const ::std::os::raw::c_void,
    ) -> ArgusObjectWrapperOrReflectiveArgsError,
>;
pub type ArgusBareProxiedScriptCallback = ::std::option::Option<
    unsafe extern "C" fn(
        params_count: usize,
        params: *mut argus_object_wrapper_t,
        data: *const ::std::os::raw::c_void,
        out_result: argus_script_callback_result_t,
    ),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ArgusProxiedScriptCallback {
    pub bare_fn: ArgusBareProxiedScriptCallback,
    pub data: *const ::std::os::raw::c_void,
}
pub type argus_bound_type_def_t = *mut ::std::os::raw::c_void;
pub type argus_bound_type_def_const_t = *const ::std::os::raw::c_void;
pub type argus_bound_enum_def_t = *mut ::std::os::raw::c_void;
pub type argus_bound_enum_def_const_t = *const ::std::os::raw::c_void;
pub type argus_bound_function_def_t = *mut ::std::os::raw::c_void;
pub type argus_bound_function_def_const_t = *const ::std::os::raw::c_void;
pub type ArgusFieldAccessor = ::std::option::Option<
    unsafe extern "C" fn(
        inst: argus_object_wrapper_const_t,
        arg1: argus_object_type_const_t,
        state: *const ::std::os::raw::c_void,
    ) -> argus_object_wrapper_t,
>;
pub type ArgusFieldMutator = ::std::option::Option<
    unsafe extern "C" fn(
        inst: argus_object_wrapper_t,
        arg1: argus_object_wrapper_t,
        state: *const ::std::os::raw::c_void,
    ),
>;
pub type argus_script_manager_t = *mut ::std::os::raw::c_void;
pub type argus_script_manager_const_t = *const ::std::os::raw::c_void;
extern "C" {
    pub fn argus_binding_error_free(err: argus_binding_error_t);
    pub fn argus_binding_error_get_type(err: argus_binding_error_const_t) -> ArgusBindingErrorType;
    pub fn argus_binding_error_get_bound_name(
        err: argus_binding_error_const_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_binding_error_get_msg(
        err: argus_binding_error_const_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_reflective_args_error_new(
        reason: *const ::std::os::raw::c_char,
    ) -> argus_reflective_args_error_t;
    pub fn argus_reflective_args_error_free(err: argus_reflective_args_error_t);
    pub fn argus_reflective_args_error_get_reason(
        err: argus_reflective_args_error_const_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_script_invocation_error_new(
        fn_name: *const ::std::os::raw::c_char,
        reason: *const ::std::os::raw::c_char,
    ) -> argus_script_invocation_error_t;
    pub fn argus_script_invocation_error_free(err: argus_script_invocation_error_t);
    pub fn argus_script_invocation_error_get_function_name(
        err: argus_script_invocation_error_const_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_script_invocation_error_get_message(
        err: argus_script_invocation_error_const_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_object_wrapper_or_refl_args_err_delete(
        res: ArgusObjectWrapperOrReflectiveArgsError,
    );
    pub fn argus_object_type_new(
        type_: ArgusIntegralType,
        size: usize,
        is_const: bool,
        is_refable: bool,
        type_id: *const ::std::os::raw::c_char,
        script_callback_type: argus_script_callback_type_const_t,
        primary_type: argus_object_type_const_t,
        secondary_type: argus_object_type_const_t,
    ) -> argus_object_type_t;
    pub fn argus_object_type_delete(obj_type: argus_object_type_t);
    pub fn argus_object_type_get_type(obj_type: argus_object_type_const_t) -> ArgusIntegralType;
    pub fn argus_object_type_get_size(obj_type: argus_object_type_const_t) -> usize;
    pub fn argus_object_type_get_is_const(obj_type: argus_object_type_const_t) -> bool;
    pub fn argus_object_type_get_is_refable(obj_type: argus_object_type_const_t) -> bool;
    pub fn argus_object_type_get_type_id(
        obj_type: argus_object_type_const_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_object_type_get_type_name(
        obj_type: argus_object_type_const_t,
    ) -> *const ::std::os::raw::c_char;
    pub fn argus_object_type_get_callback_type(
        obj_type: argus_object_type_const_t,
    ) -> argus_script_callback_type_const_t;
    pub fn argus_object_type_get_primary_type(
        obj_type: argus_object_type_const_t,
    ) -> argus_object_type_const_t;
    pub fn argus_object_type_get_secondary_type(
        obj_type: argus_object_type_const_t,
    ) -> argus_object_type_const_t;
    pub fn argus_script_callback_type_new(
        param_count: usize,
        param_types: *const argus_object_type_const_t,
        return_type: argus_object_type_const_t,
    ) -> argus_script_callback_type_t;
    pub fn argus_script_callback_type_delete(callback_type: argus_script_callback_type_t);
    pub fn argus_script_callback_type_get_param_count(
        callback_type: argus_script_callback_type_const_t,
    ) -> usize;
    pub fn argus_script_callback_type_get_params(
        callback_type: argus_script_callback_type_t,
        obj_types: *mut argus_object_type_t,
        count: usize,
    );
    pub fn argus_script_callback_type_get_return_type(
        callback_type: argus_script_callback_type_t,
    ) -> argus_object_type_t;
    pub fn argus_script_callback_result_new() -> argus_script_callback_result_t;
    pub fn argus_script_callback_result_delete(result: argus_script_callback_result_t);
    pub fn argus_script_callback_result_emplace(
        dest: argus_script_callback_result_t,
        value: argus_object_wrapper_t,
        error: argus_script_invocation_error_t,
    );
    pub fn argus_script_callback_result_is_ok(result: argus_script_callback_result_t) -> bool;
    pub fn argus_script_callback_result_get_value(
        result: argus_script_callback_result_t,
    ) -> argus_object_wrapper_t;
    pub fn argus_script_callback_result_get_error(
        result: argus_script_callback_result_t,
    ) -> argus_script_invocation_error_const_t;
    pub fn argus_object_wrapper_new(
        obj_type: argus_object_type_const_t,
        size: usize,
    ) -> argus_object_wrapper_t;
    pub fn argus_object_wrapper_delete(obj_wrapper: argus_object_wrapper_t);
    pub fn argus_object_wrapper_get_type(
        obj_wrapper: argus_object_wrapper_const_t,
    ) -> argus_object_type_const_t;
    pub fn argus_object_wrapper_get_value(
        obj_wrapper: argus_object_wrapper_const_t,
    ) -> *const ::std::os::raw::c_void;
    pub fn argus_object_wrapper_get_value_mut(
        obj_wrapper: argus_object_wrapper_t,
    ) -> *mut ::std::os::raw::c_void;
    pub fn argus_object_wrapper_is_on_heap(obj_wrapper: argus_object_wrapper_const_t) -> bool;
    pub fn argus_object_wrapper_get_buffer_size(obj_wrapper: argus_object_wrapper_const_t)
        -> usize;
    pub fn argus_object_wrapper_is_initialized(obj_wrapper: argus_object_wrapper_const_t) -> bool;
    pub fn argus_create_type_def(
        name: *const ::std::os::raw::c_char,
        size: usize,
        type_id: *const ::std::os::raw::c_char,
        is_refable: bool,
        copy_ctor: ArgusCopyCtorProxy,
        move_ctor: ArgusMoveCtorProxy,
        dtor: ArgusDtorProxy,
    ) -> argus_bound_type_def_t;
    pub fn argus_create_enum_def(
        name: *const ::std::os::raw::c_char,
        width: usize,
        type_id: *const ::std::os::raw::c_char,
    ) -> argus_bound_enum_def_t;
    pub fn argus_add_enum_value(
        def: argus_bound_enum_def_t,
        name: *const ::std::os::raw::c_char,
        value: i64,
    ) -> ArgusMaybeBindingError;
    pub fn argus_add_member_field(
        arg1: argus_bound_type_def_t,
        name: *const ::std::os::raw::c_char,
        field_type: argus_object_type_const_t,
        accessor: ArgusFieldAccessor,
        accessor_state: *const ::std::os::raw::c_void,
        mutator: ArgusFieldMutator,
        mutator_state: *const ::std::os::raw::c_void,
    ) -> ArgusMaybeBindingError;
    pub fn argus_add_member_static_function(
        def: argus_bound_type_def_t,
        name: *const ::std::os::raw::c_char,
        params_count: usize,
        params: *const argus_object_type_const_t,
        ret_type: argus_object_type_const_t,
        proxied_fn: ArgusProxiedNativeFunction,
        extra: *mut ::std::os::raw::c_void,
    ) -> ArgusMaybeBindingError;
    pub fn argus_add_member_instance_function(
        def: argus_bound_type_def_t,
        name: *const ::std::os::raw::c_char,
        is_const: bool,
        params_count: usize,
        params: *const argus_object_type_const_t,
        ret_type: argus_object_type_const_t,
        proxied_fn: ArgusProxiedNativeFunction,
        extra: *mut ::std::os::raw::c_void,
    ) -> ArgusMaybeBindingError;
    pub fn argus_create_global_function_def(
        name: *const ::std::os::raw::c_char,
        is_const: bool,
        params_count: usize,
        params: *const argus_object_type_const_t,
        ret_type: argus_object_type_const_t,
        proxied_fn: ArgusProxiedNativeFunction,
        extra: *mut ::std::os::raw::c_void,
    ) -> argus_bound_function_def_t;
    pub fn argus_bound_type_def_delete(def: argus_bound_type_def_t);
    pub fn argus_bound_enum_def_delete(def: argus_bound_enum_def_t);
    pub fn argus_bound_function_def_delete(def: argus_bound_function_def_t);
    pub fn argus_script_manager_instance() -> argus_script_manager_t;
    pub fn argus_script_manager_bind_type(
        manager: argus_script_manager_t,
        def: argus_bound_type_def_t,
    ) -> ArgusMaybeBindingError;
    pub fn argus_script_manager_bind_enum(
        manager: argus_script_manager_t,
        def: argus_bound_enum_def_t,
    ) -> ArgusMaybeBindingError;
    pub fn argus_script_manager_bind_global_function(
        manager: argus_script_manager_t,
        def: argus_bound_function_def_t,
    ) -> ArgusMaybeBindingError;
    pub fn argus_create_object_wrapper(
        ty: argus_object_type_const_t,
        ptr: *const ::std::os::raw::c_void,
        size: usize,
    ) -> ArgusObjectWrapperOrReflectiveArgsError;
    pub fn argus_copy_bound_type(
        type_id: *const ::std::os::raw::c_char,
        dst: *mut ::std::os::raw::c_void,
        src: *const ::std::os::raw::c_void,
    );
    pub fn argus_move_bound_type(
        type_id: *const ::std::os::raw::c_char,
        dst: *mut ::std::os::raw::c_void,
        src: *mut ::std::os::raw::c_void,
    );
}

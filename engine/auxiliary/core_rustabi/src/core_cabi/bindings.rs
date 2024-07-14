/* automatically generated by rust-bindgen 0.69.4 */

use super::*;

pub const ARGUS_ENGINE_NAME: &[u8; 6] = b"Argus\0";
pub const ARGUS_ENGINE_VERSION_MAJOR: u32 = 0;
pub const ARGUS_ENGINE_VERSION_MINOR: u32 = 0;
pub const ARGUS_ENGINE_VERSION_INCR: u32 = 1;
pub type Index = u64;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct argus_scripting_parameters_t {
    pub has_main: bool,
    pub main: *const ::std::os::raw::c_char,
}
#[repr(C)]
pub struct argus_initial_window_parameters_t {
    pub has_id: bool,
    pub id: *const ::std::os::raw::c_char,
    pub has_title: bool,
    pub title: *const ::std::os::raw::c_char,
    pub has_mode: bool,
    pub mode: *const ::std::os::raw::c_char,
    pub has_vsync: bool,
    pub vsync: bool,
    pub has_mouse_visible: bool,
    pub mouse_visible: bool,
    pub has_mouse_captured: bool,
    pub mouse_captured: bool,
    pub has_mouse_raw_input: bool,
    pub mouse_raw_input: bool,
    pub has_position: bool,
    pub position: argus_vector_2i_t,
    pub has_dimensions: bool,
    pub dimensions: argus_vector_2u_t,
}
pub const LIFECYCLE_STAGE_LOAD: LifecycleStage = 0;
pub const LIFECYCLE_STAGE_PREINIT: LifecycleStage = 1;
pub const LIFECYCLE_STAGE_INIT: LifecycleStage = 2;
pub const LIFECYCLE_STAGE_POSTINIT: LifecycleStage = 3;
pub const LIFECYCLE_STAGE_RUNNING: LifecycleStage = 4;
pub const LIFECYCLE_STAGE_PREDEINIT: LifecycleStage = 5;
pub const LIFECYCLE_STAGE_DEINIT: LifecycleStage = 6;
pub const LIFECYCLE_STAGE_POSTDEINIT: LifecycleStage = 7;
pub type LifecycleStage = ::std::os::raw::c_uint;
pub type lifecycle_update_callback_t =
    ::std::option::Option<unsafe extern "C" fn(arg1: LifecycleStage)>;
pub type nullary_callback_t = ::std::option::Option<unsafe extern "C" fn()>;
pub type delta_callback_t = ::std::option::Option<unsafe extern "C" fn(arg1: u64)>;
pub const ORDERING_FIRST: Ordering = 0;
pub const ORDERING_EARLY: Ordering = 1;
pub const ORDERING_STANDARD: Ordering = 2;
pub const ORDERING_LATE: Ordering = 3;
pub const ORDERING_LAST: Ordering = 4;
pub type Ordering = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ScreenSpace {
    pub left: f32,
    pub right: f32,
    pub top: f32,
    pub bottom: f32,
}
pub const SSS_MODE_NORMALIZE_MIN_DIM: ScreenSpaceScaleMode = 0;
pub const SSS_MODE_NORMALIZE_MAX_DIM: ScreenSpaceScaleMode = 1;
pub const SSS_MODE_NORMALIZE_VERTICAL: ScreenSpaceScaleMode = 2;
pub const SSS_MODE_NORMALIZE_HORIZONTAL: ScreenSpaceScaleMode = 3;
pub const SSS_MODE_NONE: ScreenSpaceScaleMode = 4;
pub type ScreenSpaceScaleMode = ::std::os::raw::c_uint;
pub type argus_event_t = *mut ::std::os::raw::c_void;
pub type argus_event_const_t = *const ::std::os::raw::c_void;
pub type argus_event_handler_t = ::std::option::Option<
    unsafe extern "C" fn(arg1: argus_event_const_t, arg2: *mut ::std::os::raw::c_void),
>;
pub type argus_event_handler_unregister_callback_t =
    ::std::option::Option<unsafe extern "C" fn(arg1: Index, arg2: *mut ::std::os::raw::c_void)>;
pub const TARGET_THREAD_UPDATE: TargetThread = 0;
pub const TARGET_THREAD_RENDER: TargetThread = 1;
pub type TargetThread = ::std::os::raw::c_uint;
pub type message_performer_t =
    ::std::option::Option<unsafe extern "C" fn(arg1: *const ::std::os::raw::c_void)>;
extern "C" {
    pub fn argus_load_client_config(config_namespace: *const ::std::os::raw::c_char);
    pub fn argus_get_client_id() -> *const ::std::os::raw::c_char;
    pub fn argus_set_client_id(id: *const ::std::os::raw::c_char);
    pub fn argus_get_client_name() -> *const ::std::os::raw::c_char;
    pub fn argus_set_client_name(id: *const ::std::os::raw::c_char);
    pub fn argus_get_client_version() -> *const ::std::os::raw::c_char;
    pub fn argus_set_client_version(id: *const ::std::os::raw::c_char);
    pub fn argus_get_scripting_parameters() -> argus_scripting_parameters_t;
    pub fn argus_set_scripting_parameters(params: *const argus_scripting_parameters_t);
    pub fn argus_get_initial_window_parameters() -> argus_initial_window_parameters_t;
    pub fn argus_set_initial_window_parameters(params: *const argus_initial_window_parameters_t);
    pub fn argus_get_default_bindings_resource_id() -> *const ::std::os::raw::c_char;
    pub fn argus_set_default_bindings_resource_id(resource_id: *const ::std::os::raw::c_char);
    pub fn argus_get_save_user_bindings() -> bool;
    pub fn argus_set_save_user_bindings(save: bool);
    pub fn argus_lifecycle_stage_to_str(stage: LifecycleStage) -> *const ::std::os::raw::c_char;
    pub fn argus_register_dynamic_module(
        id: *const ::std::os::raw::c_char,
        lifecycle_callback: ::std::option::Option<unsafe extern "C" fn(arg1: LifecycleStage)>,
        dependencies_count: usize,
        dependencies: *const *const ::std::os::raw::c_char,
    );
    pub fn argus_enable_dynamic_module(module_id: *const ::std::os::raw::c_char) -> bool;
    pub fn argus_get_present_dynamic_modules() -> StringArray;
    pub fn argus_initialize_engine();
    pub fn argus_start_engine(callback: delta_callback_t) -> !;
    pub fn argus_crash(msg: *const ::std::os::raw::c_char) -> !;
    pub fn argus_get_current_lifecycle_stage() -> LifecycleStage;
    pub fn argus_register_update_callback(
        update_callback: delta_callback_t,
        ordering: Ordering,
    ) -> Index;
    pub fn argus_unregister_update_callback(id: Index);
    pub fn argus_register_render_callback(
        render_callback: delta_callback_t,
        ordering: Ordering,
    ) -> Index;
    pub fn argus_unregister_render_callback(id: Index);
    pub fn argus_run_on_game_thread(callback: nullary_callback_t);
    pub fn argus_is_current_thread_update_thread() -> bool;
    pub fn set_target_tickrate(target_tickrate: ::std::os::raw::c_uint);
    pub fn set_target_framerate(target_framerate: ::std::os::raw::c_uint);
    pub fn set_load_modules(module_names: *const *const ::std::os::raw::c_char, count: usize);
    pub fn add_load_module(module_name: *const ::std::os::raw::c_char);
    pub fn get_preferred_render_backends(
        out_count: *mut usize,
        out_names: *mut *const ::std::os::raw::c_char,
    );
    pub fn set_render_backends(names: *const *const ::std::os::raw::c_char, count: usize);
    pub fn add_render_backend(name: *const ::std::os::raw::c_char);
    pub fn set_render_backend(name: *const ::std::os::raw::c_char);
    pub fn get_screen_space_scale_mode() -> ScreenSpaceScaleMode;
    pub fn set_screen_space_scale_mode(mode: ScreenSpaceScaleMode);
    pub fn argus_event_get_type_id(event: argus_event_const_t) -> *const ::std::os::raw::c_char;
    pub fn argus_register_event_handler(
        type_id: *const ::std::os::raw::c_char,
        handler: argus_event_handler_t,
        target_thread: TargetThread,
        data: *mut ::std::os::raw::c_void,
        ordering: Ordering,
        unregister_callback: argus_event_handler_unregister_callback_t,
    ) -> Index;
    pub fn argus_unregister_event_handler(index: Index);
    pub fn argus_dispatch_event(event: argus_event_t);
    pub fn argus_register_message_performer(
        type_id: *const ::std::os::raw::c_char,
        performer: message_performer_t,
    );
}

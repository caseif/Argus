/* automatically generated by rust-bindgen 0.69.4 */

use super::*;

pub type argus_display_t = *mut ::std::os::raw::c_void;
pub type argus_display_const_t = *const ::std::os::raw::c_void;
#[repr(C)]
pub struct argus_display_mode_t {
    pub resolution: argus_vector_2u_t,
    pub refresh_rate: u16,
    pub color_depth: argus_vector_4u_t,
    pub extra_data: u32,
}
pub type argus_window_t = *mut ::std::os::raw::c_void;
pub type argus_window_const_t = *const ::std::os::raw::c_void;
pub type argus_canvas_t = *mut ::std::os::raw::c_void;
pub type argus_canvas_const_t = *const ::std::os::raw::c_void;
pub type argus_window_callback_t =
    ::std::option::Option<unsafe extern "C" fn(arg1: argus_window_t)>;
pub type argus_canvas_ctor_t =
    ::std::option::Option<unsafe extern "C" fn(arg1: argus_window_t) -> argus_canvas_t>;
pub type argus_canvas_dtor_t = ::std::option::Option<unsafe extern "C" fn(arg1: argus_canvas_t)>;
pub const WINDOW_CREATE_FLAG_NONE: WindowCreateFlags = 0;
pub const WINDOW_CREATE_FLAG_OPENGL: WindowCreateFlags = 1;
pub const WINDOW_CREATE_FLAG_VULKAN: WindowCreateFlags = 2;
pub const WINDOW_CREATE_FLAG_METAL: WindowCreateFlags = 4;
pub const WINDOW_CREATE_FLAG_DIRECTX: WindowCreateFlags = 8;
pub const WINDOW_CREATE_FLAG_WEBGPU: WindowCreateFlags = 16;
pub const WINDOW_CREATE_FLAG_GRAPHICS_API_MASK: WindowCreateFlags = 31;
pub type WindowCreateFlags = ::std::os::raw::c_uint;
pub type gl_context_t = *mut ::std::os::raw::c_void;
pub const GL_CONTEXT_FLAG_NONE: GLContextFlags = 0;
pub const GL_CONTEXT_FLAG_PROFILE_CORE: GLContextFlags = 1;
pub const GL_CONTEXT_FLAG_PROFILE_ES: GLContextFlags = 2;
pub const GL_CONTEXT_FLAG_PROFILE_COMPAT: GLContextFlags = 4;
pub const GL_CONTEXT_FLAG_DEBUG_CONTEXT: GLContextFlags = 256;
pub const GL_CONTEXT_FLAG_PROFILE_MASK: GLContextFlags = 7;
pub type GLContextFlags = ::std::os::raw::c_uint;
pub const k_event_type_window: &[u8; 7] = b"window\0";
pub const ARGUS_WINDOW_EVENT_TYPE_CREATE: ArgusWindowEventType = 0;
pub const ARGUS_WINDOW_EVENT_TYPE_UPDATE: ArgusWindowEventType = 1;
pub const ARGUS_WINDOW_EVENT_TYPE_REQUEST_CLOSE: ArgusWindowEventType = 2;
pub const ARGUS_WINDOW_EVENT_TYPE_MINIMIZE: ArgusWindowEventType = 3;
pub const ARGUS_WINDOW_EVENT_TYPE_RESTORE: ArgusWindowEventType = 4;
pub const ARGUS_WINDOW_EVENT_TYPE_FOCUS: ArgusWindowEventType = 5;
pub const ARGUS_WINDOW_EVENT_TYPE_UNFOCUS: ArgusWindowEventType = 6;
pub const ARGUS_WINDOW_EVENT_TYPE_RESIZE: ArgusWindowEventType = 7;
pub const ARGUS_WINDOW_EVENT_TYPE_MOVE: ArgusWindowEventType = 8;
pub type ArgusWindowEventType = ::std::os::raw::c_uint;
pub type argus_window_event_t = *mut ::std::os::raw::c_void;
pub type argus_window_event_const_t = *const ::std::os::raw::c_void;
extern "C" {
    pub fn argus_display_get_available_displays(
        out_count: *mut usize,
        out_displays: *mut argus_display_const_t,
    );
    pub fn argus_display_get_name(self_: argus_display_const_t) -> *const ::std::os::raw::c_char;
    pub fn argus_display_get_position(self_: argus_display_const_t) -> argus_vector_2i_t;
    pub fn argus_display_get_display_modes(
        self_: argus_display_const_t,
        out_count: *mut usize,
        out_modes: *mut argus_display_mode_t,
    );
    pub fn argus_set_window_creation_flags(flags: WindowCreateFlags);
    pub fn argus_get_window(id: *const ::std::os::raw::c_char) -> argus_window_t;
    pub fn argus_get_window_handle(window: argus_window_const_t) -> *mut ::std::os::raw::c_void;
    pub fn argus_get_window_from_handle(handle: *const ::std::os::raw::c_void) -> argus_window_t;
    pub fn argus_window_set_canvas_ctor_and_dtor(
        ctor: argus_canvas_ctor_t,
        dtor: argus_canvas_dtor_t,
    );
    pub fn argus_window_create(
        id: *const ::std::os::raw::c_char,
        parent: argus_window_t,
    ) -> argus_window_t;
    pub fn argus_window_get_id(self_: argus_window_const_t) -> *const ::std::os::raw::c_char;
    pub fn argus_window_get_canvas(self_: argus_window_const_t) -> argus_canvas_t;
    pub fn argus_window_is_created(self_: argus_window_const_t) -> bool;
    pub fn argus_window_is_ready(self_: argus_window_const_t) -> bool;
    pub fn argus_window_is_close_request_pending(self_: argus_window_const_t) -> bool;
    pub fn argus_window_is_closed(self_: argus_window_const_t) -> bool;
    pub fn argus_window_create_child_window(
        self_: argus_window_t,
        id: *const ::std::os::raw::c_char,
    ) -> argus_window_t;
    pub fn argus_window_remove_child(self_: argus_window_t, child: argus_window_const_t);
    pub fn argus_window_update(self_: argus_window_t, delta_us: u64);
    pub fn argus_window_set_title(self_: argus_window_t, title: *const ::std::os::raw::c_char);
    pub fn argus_window_is_fullscreen(self_: argus_window_const_t) -> bool;
    pub fn argus_window_set_fullscreen(self_: argus_window_t, fullscreen: bool);
    pub fn argus_window_get_resolution(
        self_: argus_window_t,
        out_resolution: *mut argus_vector_2u_t,
        out_dirty: *mut bool,
    );
    pub fn argus_window_peek_resolution(self_: argus_window_const_t) -> argus_vector_2u_t;
    pub fn argus_window_set_windowed_resolution(self_: argus_window_t, width: u32, height: u32);
    pub fn argus_window_is_vsync_enabled(
        self_: argus_window_t,
        out_enabled: *mut bool,
        out_dirty: *mut bool,
    );
    pub fn argus_window_set_vsync_enabled(self_: argus_window_t, enabled: bool);
    pub fn argus_window_set_windowed_position(self_: argus_window_t, x: i32, y: i32);
    pub fn argus_window_get_display_affinity(self_: argus_window_const_t) -> argus_display_const_t;
    pub fn argus_window_set_display_affinity(self_: argus_window_t, display: argus_display_const_t);
    pub fn argus_window_get_display_mode(self_: argus_window_const_t) -> argus_display_mode_t;
    pub fn argus_window_set_display_mode(self_: argus_window_t, mode: argus_display_mode_t);
    pub fn argus_window_is_mouse_captured(self_: argus_window_const_t) -> bool;
    pub fn argus_window_set_mouse_captured(self_: argus_window_t, captured: bool);
    pub fn argus_window_is_mouse_visible(self_: argus_window_const_t) -> bool;
    pub fn argus_window_set_mouse_visible(self_: argus_window_t, visible: bool);
    pub fn argus_window_is_mouse_raw_input(self_: argus_window_const_t) -> bool;
    pub fn argus_window_set_mouse_raw_input(self_: argus_window_t, raw_input: bool);
    pub fn argus_window_get_content_scale(self_: argus_window_const_t) -> argus_vector_2f_t;
    pub fn argus_window_set_close_callback(
        self_: argus_window_t,
        callback: argus_window_callback_t,
    );
    pub fn argus_window_commit(self_: argus_window_t);
    pub fn argus_window_request_close(self_: argus_window_t);
    pub fn argus_gl_load_library() -> i32;
    pub fn argus_gl_unload_library();
    pub fn argus_gl_create_context(
        window: argus_window_t,
        version_major: i32,
        version_minor: i32,
        flags: GLContextFlags,
    ) -> gl_context_t;
    pub fn argus_gl_destroy_context(context: gl_context_t);
    pub fn argus_gl_is_context_current(context: gl_context_t) -> bool;
    pub fn argus_gl_make_context_current(
        window: argus_window_t,
        context: gl_context_t,
    ) -> ::std::os::raw::c_int;
    pub fn argus_gl_load_proc(name: *const ::std::os::raw::c_char) -> *mut ::std::os::raw::c_void;
    pub fn argus_gl_swap_interval(interval: i32);
    pub fn argus_gl_swap_buffers(window: argus_window_t);
    pub fn argus_vk_is_supported() -> bool;
    pub fn argus_vk_create_surface(
        window: argus_window_t,
        instance: *mut ::std::os::raw::c_void,
        out_surface: *mut *mut ::std::os::raw::c_void,
    ) -> ::std::os::raw::c_int;
    pub fn argus_vk_get_required_instance_extensions(
        window: argus_window_t,
        out_count: *mut ::std::os::raw::c_uint,
        out_names: *mut *const ::std::os::raw::c_char,
    ) -> ::std::os::raw::c_int;
    pub fn argus_window_event_get_subtype(
        self_: argus_window_event_const_t,
    ) -> ArgusWindowEventType;
    pub fn argus_window_event_get_window(self_: argus_window_event_const_t) -> argus_window_t;
    pub fn argus_window_event_get_resolution(
        self_: argus_window_event_const_t,
    ) -> argus_vector_2u_t;
    pub fn argus_window_event_get_position(self_: argus_window_event_const_t) -> argus_vector_2i_t;
    pub fn argus_window_event_get_delta_us(self_: argus_window_event_const_t) -> u64;
}

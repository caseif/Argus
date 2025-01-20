use std::{ffi, ptr};
use std::ffi::{CStr, CString};
use std::mem::MaybeUninit;
use bitflags::bitflags;
use num_enum::{IntoPrimitive, TryFromPrimitive};
use crate::bindings::*;
use crate::error::{sdl_get_error, SdlError};
use crate::rect::SdlRect;

pub const SDL_WINDOWPOS_CENTERED: u32 = SDL_WINDOWPOS_CENTERED_MASK;

pub fn sdl_windowpos_centered_display(index: u32) -> u32 {
    SDL_WINDOWPOS_CENTERED_MASK | index
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct SdlWindow {
    handle: *mut SDL_Window,
}

impl SdlWindow {
    pub fn as_ptr(&self) -> *const () {
        self.handle.cast_const().cast()
    }

    pub fn as_mut_ptr(&mut self) -> *mut () {
        self.handle.cast()
    }
    
    pub fn as_addr(&self) -> usize {
        self.handle.addr()
    }

    pub fn get_flags(&self) -> SdlWindowFlags {
        SdlWindowFlags::from_bits_retain(unsafe { SDL_GetWindowFlags(self.handle) })
    }

    pub fn get_display_index(&self) -> Result<i32, SdlError> {
        let retval = unsafe { SDL_GetWindowDisplayIndex(self.handle) };
        if retval < 0 {
            return Err(sdl_get_error());
        }
        Ok(retval)
    }
}

impl SdlWindow {
    pub fn from_id(id: u32) -> Result<Self, SdlError> {
        let window_ptr = unsafe { SDL_GetWindowFromID(id) };
        if window_ptr.is_null() {
            return Err(sdl_get_error());
        }
        Ok(Self { handle: window_ptr })
    }

    pub fn create(
        title: impl Into<String>,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        flags: SdlWindowFlags,
    ) -> Result<Self, SdlError> {
        let title_c = CString::new(title.into()).unwrap();
        let handle = unsafe { SDL_CreateWindow(title_c.as_ptr(), x, y, w, h, flags.bits()) };
        if handle.is_null() {
            return Err(sdl_get_error());
        }
        Ok(Self { handle })
    }

    pub fn get_handle(&self) -> *const ffi::c_void {
        self.handle.cast()
    }

    pub fn get_handle_mut(&mut self) -> *mut ffi::c_void {
        self.handle.cast()
    }

    pub fn set_title(&mut self, title: impl Into<String>) {
        let title = CString::new(title.into()).unwrap();
        unsafe { SDL_SetWindowTitle(self.handle, title.as_ptr()) };
    }

    pub fn set_position(&mut self, x: i32, y: i32) {
        unsafe { SDL_SetWindowPosition(self.handle, x, y) };
    }

    pub fn set_size(&mut self, w: i32, h: i32) {
        unsafe { SDL_SetWindowSize(self.handle, w, h) };
    }

    pub fn set_fullscreen(&mut self, flags: SdlWindowFlags) {
        unsafe { SDL_SetWindowFullscreen(self.handle, flags.bits()) };
    }

    pub fn set_display_mode(&mut self, mode: &SdlDisplayMode) -> Result<(), SdlError> {
        let retval = unsafe { SDL_SetWindowDisplayMode(self.handle, ptr::from_ref(&mode.into())) };
        if retval != 0 {
            return Err(sdl_get_error());
        }
        Ok(())
    }

    pub fn set_mouse_grab(&mut self, grabbed: bool) {
        unsafe { SDL_SetWindowMouseGrab(self.handle, if grabbed { SDL_TRUE } else { SDL_FALSE }) };
    }

    pub fn show(&mut self) {
        unsafe { SDL_ShowWindow(self.handle) };
    }
}

bitflags! {
    pub struct SdlWindowFlags: u32 {
        const Empty = 0;
        const Fullscreen = SDL_WINDOW_FULLSCREEN;
        const OpenGL = SDL_WINDOW_OPENGL;
        const Shown = SDL_WINDOW_SHOWN;
        const Hidden = SDL_WINDOW_HIDDEN;
        const Borderless = SDL_WINDOW_BORDERLESS;
        const Resizable = SDL_WINDOW_RESIZABLE;
        const Minimized = SDL_WINDOW_MINIMIZED;
        const Maximized = SDL_WINDOW_MAXIMIZED;
        const MouseGrabbed = SDL_WINDOW_MOUSE_GRABBED;
        const InputFocus = SDL_WINDOW_INPUT_FOCUS;
        const MouseFocus = SDL_WINDOW_MOUSE_FOCUS;
        const FullscreenDesktop = SDL_WINDOW_FULLSCREEN_DESKTOP;
        const Foreign = SDL_WINDOW_FOREIGN;
        const AllowHighdpi = SDL_WINDOW_ALLOW_HIGHDPI;
        const MouseCapture = SDL_WINDOW_MOUSE_CAPTURE;
        const AlwaysOnTop = SDL_WINDOW_ALWAYS_ON_TOP;
        const SkipTaskbar = SDL_WINDOW_SKIP_TASKBAR;
        const Utility = SDL_WINDOW_UTILITY;
        const Tooltip = SDL_WINDOW_TOOLTIP;
        const PopupMenu = SDL_WINDOW_POPUP_MENU;
        const KeyboardGrabbed = SDL_WINDOW_KEYBOARD_GRABBED;
        const Vulkan = SDL_WINDOW_VULKAN;
        const Metal = SDL_WINDOW_METAL;
    }
}

#[derive(Clone, Copy, Debug)]
pub struct SdlGlContext {
    handle: *mut ffi::c_void,
}

#[repr(u32)]
pub enum SdlGlAttribute {
    RedSize = SDL_GL_RED_SIZE,
    GreenSize = SDL_GL_GREEN_SIZE,
    BlueSize = SDL_GL_BLUE_SIZE,
    AlphaSize = SDL_GL_ALPHA_SIZE,
    BufferSize = SDL_GL_BUFFER_SIZE,
    DoubleBuffer = SDL_GL_DOUBLEBUFFER,
    DepthSize = SDL_GL_DEPTH_SIZE,
    StencilSize = SDL_GL_STENCIL_SIZE,
    AccumRedSize = SDL_GL_ACCUM_RED_SIZE,
    AccumGreenSize = SDL_GL_ACCUM_GREEN_SIZE,
    AccumBlueSize = SDL_GL_ACCUM_BLUE_SIZE,
    AccumAlphaSize = SDL_GL_ACCUM_ALPHA_SIZE,
    Stereo = SDL_GL_STEREO,
    MultisampleBuffers = SDL_GL_MULTISAMPLEBUFFERS,
    Multisamplesamples = SDL_GL_MULTISAMPLESAMPLES,
    AcceleratedVisual = SDL_GL_ACCELERATED_VISUAL,
    RetainedBacking = SDL_GL_RETAINED_BACKING,
    ContextMajorVersion = SDL_GL_CONTEXT_MAJOR_VERSION,
    ContextMinorVersion = SDL_GL_CONTEXT_MINOR_VERSION,
    ContextEgl = SDL_GL_CONTEXT_EGL,
    ContextFlags = SDL_GL_CONTEXT_FLAGS,
    ContextProfileMask = SDL_GL_CONTEXT_PROFILE_MASK,
    ShareWithCurrentContext = SDL_GL_SHARE_WITH_CURRENT_CONTEXT,
    FramebufferSrgbCapable = SDL_GL_FRAMEBUFFER_SRGB_CAPABLE,
    ContextReleaseBehavior = SDL_GL_CONTEXT_RELEASE_BEHAVIOR,
    ContextResetNotification = SDL_GL_CONTEXT_RESET_NOTIFICATION,
    ContextNoError = SDL_GL_CONTEXT_NO_ERROR,
    Floatbuffers = SDL_GL_FLOATBUFFERS,
}

#[repr(u32)]
pub enum SdlGlProfile {
    Core = SDL_GL_CONTEXT_PROFILE_CORE,
    Compatibility = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY,
    Es = SDL_GL_CONTEXT_PROFILE_ES,
}

#[repr(u32)]
pub enum SdlGlContextFlags {
    None = 0,
    Debug = SDL_GL_CONTEXT_DEBUG_FLAG,
    ForwardCompatible = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG,
    RobustAccess = SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG,
    ResetIsolation = SDL_GL_CONTEXT_RESET_ISOLATION_FLAG,
}

pub fn sdl_gl_load_library(path: impl AsRef<str>) -> Result<(), SdlError> {
    let path_c = CString::new(path.as_ref()).unwrap();
    let path_ptr = if path_c.is_empty() { ptr::null() } else { path_c.as_ptr() };
    let rc = unsafe { SDL_GL_LoadLibrary(path_ptr) as i32 };
    if rc != 0 {
        return Err(sdl_get_error());
    }
    Ok(())
}

pub fn sdl_gl_unload_library() {
    unsafe { SDL_GL_UnloadLibrary() }
}

pub fn sdl_gl_set_attribute(attr: SdlGlAttribute, value: i32) -> Result<(), SdlError> {
    let rc = unsafe { SDL_GL_SetAttribute(attr as ffi::c_uint, value) };
    if rc != 0 {
        return Err(sdl_get_error());
    }
    Ok(())
}

pub fn sdl_gl_create_context(window: &SdlWindow) -> Result<SdlGlContext, SdlError> {
    let handle = unsafe { SDL_GL_CreateContext(window.handle) };
    if handle.is_null() {
        return Err(sdl_get_error());
    }
    Ok(SdlGlContext { handle })
}

pub fn sdl_gl_destroy_context(context: SdlGlContext) {
    unsafe { SDL_GL_DeleteContext(context.handle); }
}

impl SdlGlContext {
    pub fn is_current(&self) -> bool {
        unsafe { SDL_GL_GetCurrentContext() == self.handle }
    }

    pub fn make_current(&self, window: &SdlWindow) -> Result<(), SdlError> {
        let rc = unsafe { SDL_GL_MakeCurrent(window.handle, self.handle) };
        if rc != 0 {
            return Err(sdl_get_error());
        }
        Ok(())
    }
}

pub fn sdl_gl_load_proc(name: impl AsRef<str>) -> Result<*mut ffi::c_void, SdlError> {
    let name_c = CString::new(name.as_ref()).unwrap();
    let proc: *mut ffi::c_void = unsafe { SDL_GL_GetProcAddress(name_c.as_ptr()) };
    if proc.is_null() {
        return Err(sdl_get_error());
    }
    Ok(proc)
}

pub unsafe extern "C" fn sdl_gl_load_proc_ffi(name: *const ffi::c_char) -> *mut ffi::c_void {
    SDL_GL_GetProcAddress(name)
}

pub fn sdl_gl_set_swap_interval(interval: i32) -> Result<(), SdlError> {
    let rc = unsafe { SDL_GL_SetSwapInterval(interval) };
    if rc != 0 {
        return Err(sdl_get_error());
    }
    Ok(())
}

pub fn sdl_gl_swap_buffers(window: &SdlWindow) {
    unsafe { SDL_GL_SwapWindow(window.handle); }
}

pub struct SdlVkInstance {
    handle: *mut ffi::c_void,
}

impl SdlVkInstance {
    pub fn as_ptr(&self) -> *mut ffi::c_void {
        self.handle
    }
}

pub struct SdlVkSurfaceKHR {
    handle: *mut ffi::c_void,
}

impl SdlVkSurfaceKHR {
    pub fn as_ptr(&self) -> *mut ffi::c_void {
        self.handle
    }
}

pub fn sdl_vulkan_create_surface(
    window: &SdlWindow,
    instance: &SdlVkInstance,
) -> Result<SdlVkSurfaceKHR, SdlError> {
    let mut surface: SdlVkSurfaceKHR = SdlVkSurfaceKHR { handle: ptr::null_mut() };
    let success = unsafe {
        SDL_Vulkan_CreateSurface(
            window.handle,
            instance.handle.cast(),
            ptr::from_mut::<*mut ffi::c_void>(&mut surface.handle).cast(),
        )
    };
    if success != SDL_TRUE {
        return Err(sdl_get_error());
    }
    Ok(surface)
}

pub fn sdl_vulkan_get_instance_extensions(window: &SdlWindow) -> Result<Vec<String>, SdlError> {
    let mut count: ffi::c_uint = 0;
    let rc = unsafe {
        SDL_Vulkan_GetInstanceExtensions(window.handle, &mut count, ptr::null_mut())
    };
    if rc != SDL_TRUE {
        return Err(sdl_get_error());
    }

    let mut exts_c = Vec::<*const ffi::c_char>::with_capacity(count as usize);
    exts_c.resize(count as usize, ptr::null_mut());
    let rc = unsafe {
        SDL_Vulkan_GetInstanceExtensions(window.handle, &mut count, exts_c.as_mut_ptr())
    };
    if rc != SDL_TRUE {
        return Err(sdl_get_error());
    }

    let exts = exts_c.into_iter()
        .map(|ext| unsafe { CStr::from_ptr(ext).to_str().unwrap().to_owned() })
        .collect::<Vec<_>>();
    Ok(exts)
}

#[derive(Clone, Copy, Debug, IntoPrimitive, TryFromPrimitive)]
#[repr(u32)]
pub enum SdlPixelFormat {
    Unknown = SDL_PIXELFORMAT_UNKNOWN,
    Index1Lsb = SDL_PIXELFORMAT_INDEX1LSB,
    Index1Msb = SDL_PIXELFORMAT_INDEX1MSB,
    Index4Lsb = SDL_PIXELFORMAT_INDEX4LSB,
    Index4Msb = SDL_PIXELFORMAT_INDEX4MSB,
    Index8 = SDL_PIXELFORMAT_INDEX8,
    RGB332 = SDL_PIXELFORMAT_RGB332,
    RGB444 = SDL_PIXELFORMAT_RGB444,
    BGR444 = SDL_PIXELFORMAT_BGR444,
    RGB555 = SDL_PIXELFORMAT_RGB555,
    BGR555 = SDL_PIXELFORMAT_BGR555,
    ARGB4444 = SDL_PIXELFORMAT_ARGB4444,
    RGBA4444 = SDL_PIXELFORMAT_RGBA4444,
    ABGR4444 = SDL_PIXELFORMAT_ABGR4444,
    BGRA4444 = SDL_PIXELFORMAT_BGRA4444,
    ARGB1555 = SDL_PIXELFORMAT_ARGB1555,
    RGBA5551 = SDL_PIXELFORMAT_RGBA5551,
    ABGR1555 = SDL_PIXELFORMAT_ABGR1555,
    BGRA5551 = SDL_PIXELFORMAT_BGRA5551,
    RGB565 = SDL_PIXELFORMAT_RGB565,
    BGR565 = SDL_PIXELFORMAT_BGR565,
    RGB24 = SDL_PIXELFORMAT_RGB24,
    BGR24 = SDL_PIXELFORMAT_BGR24,
    RGB888 = SDL_PIXELFORMAT_RGB888,
    RGBX8888 = SDL_PIXELFORMAT_RGBX8888,
    BGR888 = SDL_PIXELFORMAT_BGR888,
    BGRX8888 = SDL_PIXELFORMAT_BGRX8888,
    ARGB2101010 = SDL_PIXELFORMAT_ARGB2101010,
    RGBA32 = SDL_PIXELFORMAT_RGBA32,
    ARGB32 = SDL_PIXELFORMAT_ARGB32,
    BGRA32 = SDL_PIXELFORMAT_BGRA32,
    ABGR32 = SDL_PIXELFORMAT_ABGR32,
    YV12 = SDL_PIXELFORMAT_YV12,
    IYUV = SDL_PIXELFORMAT_IYUV,
    YUY2 = SDL_PIXELFORMAT_YUY2,
    UYVY = SDL_PIXELFORMAT_UYVY,
    YVYU = SDL_PIXELFORMAT_YVYU,
    NV12 = SDL_PIXELFORMAT_NV12,
    NV21 = SDL_PIXELFORMAT_NV21,
    ExternalOes = SDL_PIXELFORMAT_EXTERNAL_OES,
}

#[derive(Clone, Copy, Debug, Default)]
pub struct PixelFormatMasks {
    pub bpp: i32,
    pub red: u32,
    pub green: u32,
    pub blue: u32,
    pub alpha: u32,
}

impl SdlPixelFormat {
    pub fn get_masks(&self) -> Result<PixelFormatMasks, SdlError> {
        let mut masks = PixelFormatMasks::default();
        let rc = unsafe {
            SDL_PixelFormatEnumToMasks(
                *self as u32,
                &mut masks.bpp,
                &mut masks.red,
                &mut masks.green,
                &mut masks.blue,
                &mut masks.alpha
            )
        };
        if rc != SDL_TRUE {
            return Err(sdl_get_error());
        }
        Ok(masks)
    }
}

pub struct SdlDisplayMode {
    pub format: SdlPixelFormat,
    pub width: i32,
    pub height: i32,
    pub refresh_rate: i32,
    pub driverdata: *mut ffi::c_void,
}

impl From<SDL_DisplayMode> for SdlDisplayMode {
    fn from(value: SDL_DisplayMode) -> Self {
        Self {
            format: SdlPixelFormat::try_from(value.format).unwrap_or(SdlPixelFormat::Unknown),
            width: value.w,
            height: value.h,
            refresh_rate: value.refresh_rate,
            driverdata: value.driverdata,
        }
    }
}

impl From<SdlDisplayMode> for SDL_DisplayMode {
    fn from(value: SdlDisplayMode) -> Self {
        SDL_DisplayMode::from(&value)
    }
}

impl From<&SdlDisplayMode> for SDL_DisplayMode {
    fn from(value: &SdlDisplayMode) -> Self {
        Self {
            format: value.format.into(),
            w: value.width,
            h: value.height,
            refresh_rate: value.refresh_rate,
            driverdata: value.driverdata,
        }
    }
}

pub fn sdl_get_num_video_displays() -> Result<i32, SdlError> {
    let retval = unsafe { SDL_GetNumVideoDisplays() };
    if retval < 0 {
        return Err(sdl_get_error());
    }
    Ok(retval)
}

pub fn sdl_get_display_name(index: i32) -> Result<String, SdlError> {
    unsafe {
        let name_c = SDL_GetDisplayName(index);
        if name_c.is_null() {
            return Err(sdl_get_error());
        }
        let name = CStr::from_ptr(name_c).to_str().unwrap().to_owned();
        Ok(name)
    }
}

pub fn sdl_get_display_bounds(index: i32) -> Result<SdlRect, SdlError> {
    let mut bounds = MaybeUninit::<SDL_Rect>::uninit();
    let rc = unsafe { SDL_GetDisplayBounds(index, bounds.as_mut_ptr()) };
    if rc != 0 {
        return Err(sdl_get_error());
    }
    let bounds = unsafe { bounds.assume_init() };
    let wrapped_bounds = SdlRect::from(bounds);
    Ok(wrapped_bounds)
}

pub fn sdl_get_num_display_modes(display_index: i32) -> Result<i32, SdlError> {
    let retval = unsafe { SDL_GetNumDisplayModes(display_index) };
    if retval < 0 {
        return Err(sdl_get_error());
    }
    Ok(retval)
}

pub fn sdl_get_display_mode(display_index: i32, mode_index: i32)
    -> Result<SdlDisplayMode, SdlError> {
    let mut mode = MaybeUninit::<SDL_DisplayMode>::uninit();
    let rc = unsafe { SDL_GetDisplayMode(display_index, mode_index, mode.as_mut_ptr()) };
    if rc != 0 {
        return Err(sdl_get_error());
    }
    let mode = unsafe { mode.assume_init() };
    let wrapped_mode = SdlDisplayMode::from(mode);
    Ok(wrapped_mode)
}

pub fn sdl_get_desktop_display_mode(display_index: i32) -> Result<SdlDisplayMode, SdlError> {
    let mut mode = MaybeUninit::<SDL_DisplayMode>::uninit();
    let rc = unsafe { SDL_GetDesktopDisplayMode(display_index, mode.as_mut_ptr()) };
    if rc != 0 {
        return Err(sdl_get_error());
    }
    let mode = unsafe { mode.assume_init() };
    let wrapped_mode = SdlDisplayMode::from(mode);
    Ok(wrapped_mode)
}

pub fn sdl_get_closest_display_mode(display_index: i32, mode: &SdlDisplayMode)
    -> Result<SdlDisplayMode, SdlError> {
    let mut closest = MaybeUninit::<SDL_DisplayMode>::uninit();
    let retval = unsafe {
        SDL_GetClosestDisplayMode(
            display_index,
            ptr::from_ref(&mode.into()),
            closest.as_mut_ptr(),
        )
    };
    if retval.is_null() {
        return Err(sdl_get_error());
    }
    let closest = unsafe { closest.assume_init() };
    Ok(closest.into())
}

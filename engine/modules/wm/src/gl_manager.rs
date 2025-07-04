use crate::{Window, WindowManager};
use bitflags::bitflags;
use fragile::Fragile;
use sdl3::video::GLProfile;
use sdl3::VideoSubsystem;
use std::ffi::CStr;
use std::{ffi, mem};

use sdl3::video::GLContext as SdlGlContext;

bitflags! {
    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    pub struct GlContextFlags: u32 {
        const None            = 0x0;
        const ProfileCore     = 0x1;
        const ProfileEs       = 0x2;
        const ProfileCompat   = 0x4;
        const DebugContext    = 0x100;
        const ProfileMask     = 0x7;
    }
}

impl GlContextFlags {
    pub fn mask_profile(self) -> Self {
        self & (
            GlContextFlags::ProfileCore |
                GlContextFlags::ProfileEs |
                GlContextFlags::ProfileCompat
        )
    }
}

pub struct GlProc {
    handle: *mut ffi::c_void,
}

impl GlProc {
    pub fn as_ptr(&self) -> *mut ffi::c_void {
        self.handle
    }
}

pub struct GlContext {
    pub(crate) underlying: Fragile<SdlGlContext>,
}

impl GlContext {
    pub(crate) fn of(underlying: SdlGlContext) -> Self {
        Self { underlying: Fragile::new(underlying) }
    }

    pub fn is_current(&self) -> bool {
        self.underlying.try_get().map(|c| c.is_current()).unwrap_or(false)
    }

    pub fn make_current(&self, window: &Window) -> Result<(), String> {
        window.handle.as_ref().ok_or_else(|| "SDL window handle is not present")?
            .try_get().map_err(|_| "GL context may only be accessed from render thread")?
            .gl_make_current(self.underlying.get())
            .map_err(|err| format!("Failed to make GL context current: {:?}", err))?;
        Ok(())
    }
}

pub struct GlManager {
    video_subsystem: VideoSubsystem,
}

impl GlManager {
    pub fn new(video_subsystem: VideoSubsystem) -> Self {
        Self { video_subsystem }
    }

    pub fn load_library(&self) -> Result<(), String> {
        self.video_subsystem.gl_load_library_default().map_err(|err| err.to_string())
    }

    pub fn unload_library(&self) {
        self.video_subsystem.gl_unload_library()
    }

    pub fn create_context(
        &self,
        window: &Window,
        version_major: u8,
        version_minor: u8,
        flags: GlContextFlags,
    ) -> Result<GlContext, String> {
        let Some(handle) = window.get_handle() else {
            return Err("Window is not yet created".to_owned());
        };

        let gl_attr = self.video_subsystem.gl_attr();
        let profile_bits = flags & GlContextFlags::ProfileMask;
        if profile_bits.bits() > 1 {
            panic!("Only one GL profile flag may be set during context creation");
        }

        gl_attr.set_context_major_version(version_major);
        gl_attr.set_context_minor_version(version_minor);

        if profile_bits.contains(GlContextFlags::ProfileCore) {
            // need to request at least GL 3.2 to get a core profile
            gl_attr.set_context_profile(GLProfile::Core);
        } else if profile_bits.contains(GlContextFlags::ProfileEs) {
            gl_attr.set_context_profile(GLProfile::GLES);
        } else if profile_bits.contains(GlContextFlags::ProfileCompat) {
            gl_attr.set_context_profile(GLProfile::Compatibility);
        }

        let mut gl_context_flags = gl_attr.set_context_flags();
        if flags.contains(GlContextFlags::DebugContext) {
            gl_context_flags.debug();
        }
        gl_context_flags.set();

        // SDL doesn't support single-buffering, not sure why this is even a flag
        gl_attr.set_double_buffer(true);
        gl_attr.set_depth_size(24);
        gl_attr.set_stencil_size(8);

        let ctx = handle.gl_create_context()
            .map_err(|err| format!("Failed to create GL context: {:?}", err))?;
        Ok(GlContext::of(ctx))
    }

    pub fn load_proc(&self, name: impl AsRef<str>) -> Option<GlProc> {
        let proc = self.video_subsystem.gl_get_proc_address(name.as_ref())?;
        // SAFETY: Transmuting to a void pointer is safe in and of itself. The
        // caller is responsible for safe handling of the pointer. Notably, the
        // type of `proc` at this point is not correct in the first place for
        // any non-nullary and/or returning functions, so transmutation by the
        // caller would be necessary in any case.
        let fn_addr = unsafe { mem::transmute(proc) };
        Some(GlProc { handle: fn_addr })
    }

    pub unsafe extern "C" fn load_proc_ffi(name: *const ffi::c_char) -> *mut ffi::c_void {
        let s = CStr::from_ptr(name as *mut _);
        let proc = WindowManager::instance().get_gl_manager().unwrap()
            .load_proc(s.to_str().unwrap())
            .unwrap();
        proc.handle
    }

    pub fn set_swap_interval(&self, interval: i32) {
        self.video_subsystem.gl_set_swap_interval(interval).unwrap();
    }

    pub fn swap_buffers(&self, window: &Window) -> Result<(), String> {
        let sdl_window = window.handle.as_ref()
            .ok_or_else(|| "SDL window handle is not present")?
            .try_get().map_err(|_| "Window may not be swapped outside of the render thread")?;
        sdl_window.gl_swap_window();
        Ok(())
    }
}

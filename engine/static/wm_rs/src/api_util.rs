/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as_ptr published by
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
use bitflags::bitflags;
use std::ffi;
use fragile::Fragile;
use sdl2::video::*;
use crate::Window;

#[derive(Clone, Debug)]
pub struct GlContext {
    underlying: Fragile<SdlGlContext>,
}

pub struct GlProc {
    handle: *mut ffi::c_void,
}

impl GlProc {
    pub fn as_ptr(&self) -> *mut ffi::c_void {
        self.handle
    }
}

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

pub fn gl_load_library() -> Result<(), String> {
    sdl_gl_load_library("").map_err(|err| err.get_message().to_string())
}

pub fn gl_unload_library() {
    sdl_gl_unload_library();
}

pub fn gl_create_context(
    window: &Window,
    version_major: i32,
    version_minor: i32,
    flags: GlContextFlags
) -> Result<GlContext, String> {
    let profile_bits = flags & GlContextFlags::ProfileMask;
    if profile_bits.bits() > 1 {
        panic!("Only one GL profile flag may be set during context creation");
    }

    sdl_gl_set_attribute(SdlGlAttribute::ContextMajorVersion, version_major).unwrap();
    sdl_gl_set_attribute(SdlGlAttribute::ContextMinorVersion, version_minor).unwrap();

    if profile_bits.contains(GlContextFlags::ProfileCore) {
        // need to request at least GL 3.2 to get a core profile
        sdl_gl_set_attribute(SdlGlAttribute::ContextProfileMask, SdlGlProfile::Core as i32)
            .unwrap();
    } else if profile_bits.contains(GlContextFlags::ProfileEs) {
        sdl_gl_set_attribute(SdlGlAttribute::ContextProfileMask, SdlGlProfile::Es as i32)
            .unwrap();
    } else if profile_bits.contains(GlContextFlags::ProfileCompat) {
        sdl_gl_set_attribute(SdlGlAttribute::ContextProfileMask, SdlGlProfile::Compatibility as i32)
            .unwrap();
    }

    let mut context_flags = SdlGlContextFlags::None;
    if flags.contains(GlContextFlags::DebugContext) {
        context_flags = SdlGlContextFlags::Debug;
    }
    // SDL doesn't support single-buffering, not sure why this is even a flag
    sdl_gl_set_attribute(SdlGlAttribute::DoubleBuffer, 1).unwrap();
    sdl_gl_set_attribute(SdlGlAttribute::ContextFlags, context_flags as i32).unwrap();
    sdl_gl_set_attribute(SdlGlAttribute::DepthSize, 24).unwrap();
    sdl_gl_set_attribute(SdlGlAttribute::StencilSize, 8).unwrap();

    let context = sdl_gl_create_context(
        window.get_handle().expect("Window handle is missing")
    )
        .map_err(|err| err.get_message().to_string())?;
    Ok(GlContext { underlying: Fragile::new(context) })
}

pub fn gl_destroy_context(context: GlContext) {
    sdl_gl_destroy_context(*context.underlying.get());
}

pub fn gl_is_context_current(context: &GlContext) -> bool {
    context.underlying.get().is_current()
}

pub fn gl_make_context_current(window: &Window, context: &GlContext) -> Result<(), String> {
    match context.underlying.get().make_current(
        window.get_handle().expect("Window handle is missing")
    ) {
        Ok(_) => Ok(()),
        _ => Err(String::from("Failed to make context current")),
    }
}

pub fn gl_load_proc(name: impl AsRef<str>) -> Result<GlProc, String> {
    let proc = sdl_gl_load_proc(name).map_err(|err| err.get_message().to_string())?;
    Ok(GlProc { handle: proc })
}

pub unsafe extern "C" fn gl_load_proc_ffi(name: *const ffi::c_char) -> *mut ffi::c_void {
    sdl_gl_load_proc_ffi(name)
}

pub fn gl_swap_interval(interval: i32) {
    sdl_gl_set_swap_interval(interval).unwrap();
}

pub fn gl_swap_buffers(window: &Window) {
    sdl_gl_swap_buffers(window.get_handle().expect("Window handle is missing"));
}

pub struct VkInstance {
    underlying: Fragile<SdlVkInstance>,
}

impl VkInstance {
    pub fn as_ptr(&self) -> *mut ffi::c_void {
        self.underlying.get().as_ptr()
    }
}

pub struct VkSurfaceKHR {
    underlying: SdlVkSurfaceKHR,
}

impl VkSurfaceKHR {
    pub fn as_ptr(&self) -> *mut ffi::c_void {
        self.underlying.as_ptr()
    }
}

pub fn vk_is_supported() -> bool {
    true //TODO
}

pub fn vk_create_surface(
    window: &Window,
    instance: &VkInstance,
) -> Result<VkSurfaceKHR, String> {
    match sdl_vulkan_create_surface(
        window.get_handle().expect("Window handle is missing"),
        instance.underlying.get(),
    ) {
        Ok(surface) => Ok(VkSurfaceKHR { underlying: surface }),
        Err(err) => Err(err.get_message().to_string()),
    }
}

pub fn vk_get_required_instance_extensions(window: &Window) -> Result<Vec<String>, String> {
    sdl_vulkan_get_instance_extensions(
        window.get_handle().expect("Window handle is missing")
    )
        .map_err(|err| err.get_message().to_string())
}

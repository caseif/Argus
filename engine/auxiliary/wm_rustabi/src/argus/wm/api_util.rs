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

use std::ffi::{c_char, c_void};
use std::ptr::null_mut;

use lowlevel_rustabi::util::*;
use num_enum::{IntoPrimitive, TryFromPrimitive};

use crate::argus::wm::Window;
use crate::wm_cabi::*;

pub struct GLContext {
    handle: gl_context_t,
}

pub struct VkInstance {
    handle: *mut c_void,
}

impl VkInstance {
    pub fn of(handle: *mut c_void) -> Self {
        return VkInstance { handle };
    }
}

pub struct VkSurface {
    handle: *mut c_void,
}

impl VkSurface {
    fn of(handle: *mut c_void) -> Self {
        return VkSurface { handle };
    }

    pub unsafe fn get_handle(&self) -> *const c_void {
        return self.handle;
    }

    pub unsafe fn get_handle_mut(&mut self) -> *mut c_void {
        return self.handle;
    }
}

#[derive(Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, TryFromPrimitive)]
#[repr(u32)]
pub enum GLContextFlags {
    None = GL_CONTEXT_FLAG_NONE,
    ProfileCore = GL_CONTEXT_FLAG_PROFILE_CORE,
    ProfileES = GL_CONTEXT_FLAG_PROFILE_ES,
    ProfileCompat = GL_CONTEXT_FLAG_PROFILE_COMPAT,
    DebugContext = GL_CONTEXT_FLAG_DEBUG_CONTEXT,
    ProfileMask = GL_CONTEXT_FLAG_PROFILE_MASK,
}

pub fn gl_load_library() -> i32 {
    unsafe {
        return argus_gl_load_library() as i32;
    }
}

pub fn gl_unload_library() {
    unsafe {
        argus_gl_unload_library();
    }
}

pub fn gl_create_context(
    window: &mut Window,
    version_major: i32,
    version_minor: i32,
    flags: GLContextFlags,
) -> Option<GLContext> {
    unsafe {
        let handle = argus_gl_create_context(
            window.get_handle_mut(),
            version_major,
            version_minor,
            flags.into(),
        );
        handle.as_mut().map(|p| GLContext { handle: p })
    }
}

pub fn gl_destroy_context(context: &GLContext) {
    unsafe {
        argus_gl_destroy_context(context.handle);
    }
}

pub fn gl_is_context_current(context: &GLContext) -> bool {
    unsafe {
        return argus_gl_is_context_current(context.handle);
    }
}

pub fn gl_make_context_current(window: &mut Window, context: &GLContext) -> Result<(), i32> {
    unsafe {
        match argus_gl_make_context_current(window.get_handle_mut(), context.handle) {
            0 => Ok(()),
            rc => Err(rc),
        }
    }
}

pub use crate::wm_cabi::argus_gl_load_proc as gl_load_proc;

pub fn gl_swap_interval(interval: i32) {
    unsafe {
        argus_gl_swap_interval(interval);
    }
}

pub fn gl_swap_buffers(window: &mut Window) {
    unsafe {
        argus_gl_swap_buffers(window.get_handle_mut());
    }
}

pub fn vk_is_supported() -> bool {
    unsafe {
        return argus_vk_is_supported();
    }
}

pub fn vk_create_surface(window: &mut Window, instance: &VkInstance) -> VkSurface {
    unsafe {
        let mut surface: *mut c_void = null_mut();
        argus_vk_create_surface(window.get_handle_mut(), instance.handle, &mut surface);
        return VkSurface::of(surface);
    }
}

pub fn vk_get_required_instance_extensions(window: &mut Window) -> Vec<String> {
    unsafe {
        let mut count: u32 = 0;
        argus_vk_get_required_instance_extensions(window.get_handle_mut(), &mut count, null_mut());
        let mut exts = Vec::<*const c_char>::new();
        exts.reserve(count as usize);
        argus_vk_get_required_instance_extensions(
            window.get_handle_mut(),
            null_mut(),
            exts.as_mut_ptr(),
        );
        return exts.into_iter().map(|s| cstr_to_string(s)).collect();
    }
}

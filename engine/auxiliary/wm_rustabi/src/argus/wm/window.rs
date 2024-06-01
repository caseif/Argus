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

use std::ffi::c_void;
use std::ptr::{null, null_mut};
use std::time::Duration;
use num_enum::{IntoPrimitive, TryFromPrimitive};

use lowlevel_rustabi::argus::lowlevel::{ValueAndDirtyFlag, Vector2f, Vector2u};
use lowlevel_rustabi::util::*;

use crate::wm_cabi;
use crate::wm_cabi::*;

pub struct Window {
    handle: argus_window_t,
}

pub type CanvasPtr = argus_canvas_t;

pub type WindowCallback = argus_window_callback_t;
pub type CanvasCtor = argus_canvas_ctor_t;
pub type CanvasDtor = argus_canvas_dtor_t;

#[derive(Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, TryFromPrimitive)]
#[repr(u32)]
pub enum WindowCreateFlags {
    None = WINDOW_CREATE_FLAG_NONE,
    OpenGL = WINDOW_CREATE_FLAG_OPENGL,
    Vulkan = WINDOW_CREATE_FLAG_VULKAN,
    Metal = WINDOW_CREATE_FLAG_METAL,
    DirectX = WINDOW_CREATE_FLAG_DIRECTX,
    WebGPU = WINDOW_CREATE_FLAG_WEBGPU,
    GraphicsApiMask = WINDOW_CREATE_FLAG_GRAPHICS_API_MASK,
}

pub fn set_window_create_flags(flags: WindowCreateFlags) {
    unsafe {
        argus_set_window_creation_flags(flags.into());
    }
}

pub fn get_window(id: &str) -> Option<Window> {
    unsafe {
        return argus_get_window(str_to_cstring(id).as_ptr()).as_mut().map(|ptr| Window::of(ptr));
    }
}

pub fn get_window_handle(window: &mut Window) -> *mut c_void {
    return window.get_mut_handle();
}

pub fn get_window_from_handle(handle: *const c_void) -> Option<Window> {
    unsafe {
        return argus_get_window_from_handle(handle).as_mut().map(|ptr| Window::of(ptr));
    }
}

impl Window {
    fn of(handle: argus_window_t) -> Self{
        return Self { handle };
    }

    pub(crate) fn get_handle(&self) -> argus_window_const_t {
        return self.handle;
    }

    pub(crate) fn get_mut_handle(&mut self) -> argus_window_t {
        return self.handle;
    }

    pub fn set_canvas_ctor_and_dtor(ctor: CanvasCtor, dtor: CanvasDtor) {
        unsafe {
            argus_window_set_canvas_ctor_and_dtor(ctor, dtor);
        }
    }

    pub fn create(id: &str, parent: Option<Window>) -> Self {
        unsafe {
            return Window::of(argus_window_create(str_to_cstring(id).as_ptr(), match parent {
                Some(v) => v.get_handle() as argus_window_t,
                None => null_mut(),
            }));
        }
    }

    pub fn get_id(&self) -> String {
        unsafe {
            return cstr_to_string(argus_window_get_id(self.get_handle()));
        }
    }

    pub fn get_canvas(&self) -> CanvasPtr {
        unsafe {
            return argus_window_get_canvas(self.get_handle());
        }
    }

    pub fn is_created(&self) -> bool {
        unsafe {
            return argus_window_is_created(self.get_handle());
        }
    }

    pub fn is_ready(&self) -> bool {
        unsafe {
            return argus_window_is_ready(self.get_handle());
        }
    }

    pub fn is_closed(&self) -> bool {
        unsafe {
            return argus_window_is_closed(self.get_handle());
        }
    }

    pub fn create_child_window(&mut self, id: &str) -> Window {
        unsafe {
            return Window::of(argus_window_create_child_window(self.get_mut_handle(), str_to_cstring(id).as_ptr()));
        }
    }

    pub fn remove_child(&mut self, child: &Window) {
        unsafe {
            argus_window_remove_child(self.get_mut_handle(), child.handle);
        }
    }

    pub fn update(&mut self, delta: Duration) {
        unsafe {
            argus_window_update(self.get_mut_handle(), delta.as_micros() as u64);
        }
    }

    pub fn set_title(&mut self, title: &str) {
        unsafe {
            argus_window_set_title(self.get_mut_handle(), str_to_cstring(title).as_ptr());
        }
    }

    pub fn is_fullscreen(&self) -> bool {
        unsafe {
            return argus_window_is_fullscreen(self.get_handle());
        }
    }

    pub fn set_fullscreen(&mut self, fullscreen: bool) {
        unsafe {
            argus_window_set_fullscreen(self.get_mut_handle(), fullscreen);
        }
    }

    pub fn get_resolution(&mut self) -> ValueAndDirtyFlag<Vector2u> {
        unsafe {
            let mut res: ValueAndDirtyFlag<Vector2u> = std::mem::zeroed();
            argus_window_get_resolution(self.get_mut_handle(), &mut res.value, &mut res.dirty);
            return res;
        }
    }

    pub fn peek_resolution(&self) -> Vector2u {
        unsafe {
            return argus_window_peek_resolution(self.handle);
        }
    }

    pub fn set_windowed_resolution(&mut self, width: u32, height: u32) {
        unsafe {
            argus_window_set_windowed_resolution(self.get_mut_handle(), width, height);
        }
    }

    #[allow(clippy::wrong_self_convention)]
    pub fn is_vsync_enabled(&mut self) -> ValueAndDirtyFlag<bool> {
        unsafe {
            let mut res: ValueAndDirtyFlag<bool> = std::mem::zeroed();
            argus_window_is_vsync_enabled(self.get_mut_handle(), &mut res.value, &mut res.dirty);
            return res;
        }
    }

    pub fn set_vsync_enabled(&mut self, enabled: bool) {
        unsafe {
            argus_window_set_vsync_enabled(self.get_mut_handle(), enabled);
        }
    }

    pub fn set_windowed_position(&mut self, x: i32, y: i32) {
        unsafe {
            argus_window_set_windowed_position(self.get_mut_handle(), x, y);
        }
    }

    //TODO: get_display_affinity

    //TODO: set_display_affinity

    //TODO: get_display_mode

    //TODO: set_display_mode

    pub fn is_mouse_captured(&self) -> bool {
        unsafe {
            return argus_window_is_mouse_captured(self.get_handle());
        }
    }

    pub fn set_mouse_captured(&mut self, captured: bool) {
        unsafe {
            argus_window_set_mouse_captured(self.get_mut_handle(), captured);
        }
    }

    pub fn is_mouse_visible(&self) -> bool {
        unsafe {
            return argus_window_is_mouse_visible(self.get_handle());
        }
    }

    pub fn set_mouse_visible(&mut self, visible: bool) {
        unsafe {
            argus_window_set_mouse_visible(self.get_mut_handle(), visible);
        }
    }

    pub fn is_mouse_raw_input(&self) -> bool {
        unsafe {
            return argus_window_is_mouse_raw_input(self.get_handle());
        }
    }

    pub fn set_mouse_raw_input(&mut self, raw_input: bool) {
        unsafe {
            argus_window_set_mouse_raw_input(self.get_mut_handle(), raw_input);
        }
    }

    pub fn get_content_scale(&self) -> Vector2f {
        unsafe {
            return argus_window_get_content_scale(self.get_handle());
        }
    }

    pub fn set_close_callback(&mut self, callback: WindowCallback) {
        unsafe {
            argus_window_set_close_callback(self.get_mut_handle(), callback);
        }
    }

    pub fn commit(&mut self) {
        unsafe {
            argus_window_commit(self.get_mut_handle());
        }
    }

    pub fn request_close(&mut self) {
        unsafe {
            argus_window_request_close(self.get_mut_handle());
        }
    }
}

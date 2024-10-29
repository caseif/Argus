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
use std::ptr;
use crate::argus::render::{AttachedViewport, AttachedViewport2d, Camera2d, Viewport};
use crate::render_cabi::*;
use lowlevel_rustabi::util::str_to_cstring;
use wm_rustabi::argus::wm::Window;
use wm_rustabi::wm_cabi::argus_canvas_t;

pub struct Canvas {
    handle: argus_canvas_t,
}

impl Canvas {
    pub fn of(handle: argus_canvas_t) -> Self {
        Self { handle }
    }
    
    pub fn get_window(&self) -> Window {
        unsafe { Window::of(argus_canvas_get_window(self.handle)) }
    }

    pub fn get_viewports_2d(&self) -> Vec<AttachedViewport2d> {
        unsafe {
            let count = argus_canvas_get_viewports_2d_count(self.handle);

            let mut viewport_handles: Vec<argus_attached_viewport_2d_t> = Vec::with_capacity(count);
            viewport_handles.resize(count, ptr::null_mut());
            argus_canvas_get_viewports_2d(self.handle, viewport_handles.as_mut_ptr(), count);

            viewport_handles
                .iter()
                .map(|handle| AttachedViewport2d::of(*handle))
                .collect()
        }
    }

    pub fn find_viewport(&self, id: &str) -> AttachedViewport {
        unsafe {
            let id_c = str_to_cstring(id);
            AttachedViewport::of(argus_canvas_find_viewport(self.handle, id_c.as_ptr()))
        }
    }

    pub fn attach_viewport_2d(
        &mut self,
        id: &str,
        viewport: Viewport,
        camera: &Camera2d,
        z_index: u32,
    ) -> AttachedViewport {
        unsafe {
            let id_c = str_to_cstring(id);

            AttachedViewport::of(argus_canvas_attach_viewport_2d(
                self.handle,
                id_c.as_ptr(),
                viewport.into(),
                camera.get_handle(),
                z_index,
            ))
        }
    }

    pub fn attach_default_viewport_2d(
        &mut self,
        id: &str,
        camera: &Camera2d,
        z_index: u32,
    ) -> AttachedViewport {
        unsafe {
            let id_c = str_to_cstring(id);

            AttachedViewport::of(argus_canvas_attach_default_viewport_2d(
                self.handle,
                id_c.as_ptr(),
                camera.get_handle(),
                z_index,
            ))
        }
    }

    pub fn detach_viewport_2d(&mut self, id: &str) {
        unsafe {
            let id_c = str_to_cstring(id);
            argus_canvas_detach_viewport_2d(self.handle, id_c.as_ptr());
        }
    }
}

unsafe impl Send for Canvas {}

unsafe impl Sync for Canvas {}

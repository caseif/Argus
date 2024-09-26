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

use std::ffi::c_char;
use std::ptr;
use num_enum::UnsafeFromPrimitive;
use lowlevel_rustabi::util::{cstr_to_string, str_to_cstring};

use crate::argus::render::{AttachedViewport2d, SceneType, Viewport};
use crate::render_cabi::*;

#[derive(Clone)]
pub struct AttachedViewport {
    ffi_handle: argus_attached_viewport_t,
}

impl AttachedViewport {
    pub(crate) fn of(handle: argus_attached_viewport_t) -> Self {
        Self { ffi_handle: handle }
    }
    
    pub fn as_2d(&self) -> AttachedViewport2d {
        AttachedViewport2d::of(self.ffi_handle)
    }

    pub fn get_id(&self) -> u32 {
        unsafe { argus_attached_viewport_get_id(self.ffi_handle) }
    }

    pub fn get_type(&self) -> SceneType {
        unsafe {
            SceneType::unchecked_transmute_from(argus_attached_viewport_get_type(self.ffi_handle))
        }
    }

    pub fn get_viewport(&self) -> Viewport {
        unsafe { argus_attached_viewport_get_viewport(self.ffi_handle).into() }
    }

    pub fn get_z_index(&self) -> u32 {
        unsafe { argus_attached_viewport_get_z_index(self.ffi_handle) }
    }

    pub fn get_postprocessing_shaders(&self) -> Vec<String> {
        unsafe {
            let count = argus_attached_viewport_get_postprocessing_shaders_count(self.ffi_handle);
            let mut shader_uids: Vec<*const c_char> = Vec::with_capacity(count);
            shader_uids.resize(count, ptr::null());
            argus_attached_viewport_get_postprocessing_shaders(
                self.ffi_handle,
                shader_uids.as_mut_ptr(),
                count,
            );
            shader_uids.iter().map(|str| cstr_to_string(*str)).collect()
        }
    }

    pub fn add_postprocessing_shader(&mut self, shader_uid: &str) {
        unsafe {
            let uid_c = str_to_cstring(shader_uid);
            argus_attached_viewport_add_postprocessing_shader(self.ffi_handle, uid_c.as_ptr())
        }
    }

    pub fn remove_postprocessing_shader(&mut self, shader_uid: &str) {
        unsafe {
            let uid_c = str_to_cstring(shader_uid);
            argus_attached_viewport_remove_postprocessing_shader(self.ffi_handle, uid_c.as_ptr())
        }
    }
}

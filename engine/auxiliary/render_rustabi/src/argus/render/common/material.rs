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
use lowlevel_rustabi::util::{cstr_to_str, CStringArray};
use std::ffi::{c_char, CString};
use std::{ptr, slice};
use lowlevel_rustabi::argus::lowlevel::FfiWrapper;
use crate::render_cabi::*;

pub struct Material {
    pub handle: argus_material_t,
    pub len: usize,
    must_delete: bool,
}

impl FfiWrapper for Material {
    fn of(handle: argus_material_t) -> Self {
        Self {
            handle,
            len: unsafe { argus_material_len() },
            must_delete: false,
        }
    }
}

impl Material {
    pub fn new(texture_uid: String, shader_uids: Vec<String>) -> Self {
        unsafe {
            let texture_uid_c = CString::new(texture_uid).unwrap();
            let shader_uids_c = CStringArray::new(
                shader_uids.into_iter().map(|uid| CString::new(uid).unwrap()).collect()
            );
            let handle = argus_material_new(
                texture_uid_c.as_ptr(),
                shader_uids_c.len(),
                shader_uids_c.as_ptr(),
            );
            Self {
                handle,
                len: argus_material_len(),
                must_delete: true,
            }
        }
    }

    pub fn as_slice(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self.handle.cast(), argus_material_get_ffi_size()) }
    }

    pub unsafe fn leak(&mut self) {
        self.must_delete = false;
    }

    pub fn get_texture_uid(&self) -> &str {
        unsafe { cstr_to_str(argus_material_get_texture_uid(self.handle)) }
    }

    pub fn get_shader_uids(&self) -> Vec<&str> {
        unsafe {
            let count = argus_material_get_shader_uids_count(self.handle);

            let mut cstrs: Vec<*const c_char> = Vec::with_capacity(count);
            cstrs.resize(count, ptr::null());
            argus_material_get_shader_uids(self.handle, cstrs.as_mut_ptr(), count);

            cstrs.into_iter().map(|s| cstr_to_str(s)).collect()
        }
    }
}

impl Drop for Material {
    fn drop(&mut self) {
        if self.must_delete {
            unsafe { argus_material_delete(self.handle) }
        }
    }
}

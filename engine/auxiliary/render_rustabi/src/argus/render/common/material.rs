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
use lowlevel_rustabi::util::cstr_to_str;
use std::ffi::c_char;
use std::ptr;
use lowlevel_rustabi::argus::lowlevel::FfiWrapper;
use crate::render_cabi::*;

pub struct Material {
    handle: argus_material_t,
}

impl FfiWrapper for Material {
    fn of(handle: argus_material_t) -> Self {
        Self { handle }
    }
}

impl Material {
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

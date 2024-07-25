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
use crate::render_cabi::*;
use std::{ptr, slice};

use lowlevel_rustabi::util::{cstr_to_string, str_to_cstring};

use num_enum::{IntoPrimitive, UnsafeFromPrimitive};

pub struct Shader {
    handle: argus_shader_t,
}

pub struct ShaderReflectionInfo {
    handle: argus_shader_refl_info_t,
}

#[repr(u32)]
#[derive(Debug, Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, UnsafeFromPrimitive)]
pub enum ShaderStage {
    Vertex = ARGUS_SHADER_STAGE_VERTEX,
    Fragment = ARGUS_SHADER_STAGE_FRAGMENT,
}

impl Shader {
    pub(crate) fn of(handle: argus_shader_t) -> Self {
        Self { handle }
    }

    pub(crate) fn get_handle(&self) -> argus_shader_const_t {
        self.handle
    }

    pub fn new(uid: &str, shader_type: &str, stage: ShaderStage, src: &[u8]) -> Self {
        unsafe {
            let uid_c = str_to_cstring(uid);
            let shader_type_c = str_to_cstring(shader_type);
            Self::of(argus_shader_new(
                uid_c.as_ptr(),
                shader_type_c.as_ptr(),
                stage as ArgusShaderStage,
                src.as_ptr(),
                src.len(),
            ))
        }
    }

    pub fn destroy(&mut self) {
        unsafe {
            argus_shader_delete(self.handle);
        }
    }

    pub fn get_uid(&self) -> String {
        unsafe { cstr_to_string(argus_shader_get_uid(self.handle)) }
    }

    pub fn get_type(&self) -> String {
        unsafe { cstr_to_string(argus_shader_get_type(self.handle)) }
    }

    pub fn get_stage(&self) -> ShaderStage {
        unsafe { ShaderStage::unchecked_transmute_from(argus_shader_get_stage(self.handle)) }
    }

    pub fn get_source(&self) -> &[u8] {
        unsafe {
            let mut src_ptr: *const u8 = ptr::null();
            let mut src_len: usize = 0;
            argus_shader_get_source(
                self.handle,
                ptr::addr_of_mut!(src_ptr),
                ptr::addr_of_mut!(src_len),
            );
            slice::from_raw_parts(src_ptr, src_len)
        }
    }
}

impl ShaderReflectionInfo {
    pub(crate) fn of(handle: argus_shader_refl_info_t) -> Self {
        Self { handle }
    }

    pub fn destroy(&mut self) {
        unsafe {
            argus_shader_refl_info_delete(self.handle);
        }
    }

    pub fn has_attr(&self, name: &str) -> bool {
        unsafe {
            let name_c = str_to_cstring(name);
            argus_shader_refl_info_has_attr(self.handle, name_c.as_ptr())
        }
    }

    pub fn get_attr_loc(&self, name: &str) -> Option<u32> {
        unsafe {
            let mut found = false;

            let name_c = str_to_cstring(name);

            let loc = argus_shader_refl_info_get_attr_loc(
                self.handle,
                name_c.as_ptr(),
                ptr::addr_of_mut!(found),
            );

            if found {
                Some(loc)
            } else {
                None
            }
        }
    }

    pub fn set_attr_loc(&mut self, name: &str, loc: u32) {
        unsafe {
            let name_c = str_to_cstring(name);
            argus_shader_refl_info_set_attr_loc(self.handle, name_c.as_ptr(), loc);
        }
    }

    pub fn has_output(&self, name: &str) -> bool {
        unsafe {
            let name_c = str_to_cstring(name);
            argus_shader_refl_info_has_output(self.handle, name_c.as_ptr())
        }
    }

    pub fn get_output_loc(&self, name: &str) -> Option<u32> {
        unsafe {
            let mut found = false;

            let name_c = str_to_cstring(name);

            let loc = argus_shader_refl_info_get_output_loc(
                self.handle,
                name_c.as_ptr(),
                ptr::addr_of_mut!(found),
            );

            if found {
                Some(loc)
            } else {
                None
            }
        }
    }

    pub fn set_output_loc(&mut self, name: &str, loc: u32) {
        unsafe {
            let name_c = str_to_cstring(name);
            argus_shader_refl_info_set_output_loc(self.handle, name_c.as_ptr(), loc);
        }
    }

    pub fn has_uniform_var(&self, name: &str) -> bool {
        unsafe {
            let name_c = str_to_cstring(name);
            argus_shader_refl_info_has_uniform_var(self.handle, name_c.as_ptr())
        }
    }

    pub fn has_uniform(&self, ubo: &str, name: &str) -> bool {
        unsafe {
            let ubo_c = str_to_cstring(ubo);
            let name_c = str_to_cstring(name);

            argus_shader_refl_info_has_uniform(self.handle, ubo_c.as_ptr(), name_c.as_ptr())
        }
    }

    pub fn get_uniform_var_loc(&self, name: &str) -> Option<u32> {
        unsafe {
            let mut found = false;

            let name_c = str_to_cstring(name);

            let loc = argus_shader_refl_info_get_uniform_var_loc(
                self.handle,
                name_c.as_ptr(),
                ptr::addr_of_mut!(found),
            );

            if found {
                Some(loc)
            } else {
                None
            }
        }
    }

    pub fn get_uniform_loc(&self, ubo: &str, name: &str) -> Option<u32> {
        unsafe {
            let mut found = false;

            let ubo_c = str_to_cstring(ubo);
            let name_c = str_to_cstring(name);

            let loc = argus_shader_refl_info_get_uniform_loc(
                self.handle,
                ubo_c.as_ptr(),
                name_c.as_ptr(),
                ptr::addr_of_mut!(found),
            );

            if found {
                Some(loc)
            } else {
                None
            }
        }
    }

    pub fn set_uniform_var_loc(&mut self, name: &str, loc: u32) {
        unsafe {
            let name_c = str_to_cstring(name);
            argus_shader_refl_info_set_uniform_var_loc(self.handle,name_c.as_ptr(), loc);
        }
    }

    pub fn set_uniform_loc(&mut self, ubo: &str, name: &str, loc: u32) {
        unsafe {
            let ubo_c = str_to_cstring(ubo);
            let name_c = str_to_cstring(name);

            argus_shader_refl_info_set_uniform_loc(
                self.handle,
                ubo_c.as_ptr(),
                name_c.as_ptr(),
                loc,
            );
        }
    }

    pub fn has_ubo(&self, name: &str) -> bool {
        unsafe {
            let name_c = str_to_cstring(name);
            argus_shader_refl_info_has_ubo(self.handle, name_c.as_ptr())
        }
    }

    pub fn get_ubo_binding(&self, name: &str) -> Option<u32> {
        unsafe {
            let mut found = false;

            let name_c = str_to_cstring(name);

            let binding = argus_shader_refl_info_get_ubo_binding(
                self.handle,
                name_c.as_ptr(),
                ptr::addr_of_mut!(found),
            );

            if found {
                Some(binding)
            } else {
                None
            }
        }
    }

    pub fn set_ubo_binding(&mut self, name: &str, binding: u32) {
        unsafe {
            let name_c = str_to_cstring(name);
            argus_shader_refl_info_set_attr_loc(self.handle, name_c.as_ptr(), binding);
        }
    }

    pub fn get_ubo_instance_name(&self, name: &str) -> Option<String> {
        unsafe {
            let mut found = false;

            let name_c = str_to_cstring(name);

            let inst_name = argus_shader_refl_info_get_ubo_instance_name(
                self.handle,
                name_c.as_ptr(),
                ptr::addr_of_mut!(found),
            );

            if found {
                Some(cstr_to_string(inst_name).to_string())
            } else {
                None
            }
        }
    }

    pub fn set_ubo_instance_name(&mut self, ubo_name: &str, instance_name: &str) {
        unsafe {
            let ubo_name_c = str_to_cstring(ubo_name);
            let instance_name_c = str_to_cstring(instance_name);

            argus_shader_refl_info_set_ubo_instance_name(
                self.handle,
                ubo_name_c.as_ptr(),
                instance_name_c.as_ptr(),
            );
        }
    }
}

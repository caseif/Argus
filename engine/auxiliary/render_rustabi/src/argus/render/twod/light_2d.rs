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

use num_enum::{IntoPrimitive, UnsafeFromPrimitive};
use argus_macros::ffi_repr;
use lowlevel_rustabi::argus::lowlevel::{Handle, Vector3f};
use crate::argus::render::Transform2d;
use crate::render_cabi::*;

pub struct Light2d {
    ffi_handle: argus_light_2d_t,
}

#[repr(u32)]
#[derive(Debug, Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, UnsafeFromPrimitive)]
pub enum Light2dType {
    Point = ARGUS_LIGHT_2D_TYPE_POINT,
}

#[repr(C)]
#[ffi_repr(ArgusLight2dParameters)]
#[derive(Clone, Copy, Debug)]
pub struct Light2dParameters {
    pub intensity: f32,
    pub falloff_gradient: u32,
    pub falloff_multiplier: f32,
    pub falloff_buffer: f32,
    pub shadow_falloff_gradient: u32,
    pub shadow_falloff_multiplier: f32,
}

impl Light2d {
    pub(crate) fn of(handle: argus_light_2d_t) -> Self {
        Self { ffi_handle: handle }
    }
    
    pub fn get_handle(&self) -> Handle {
        unsafe { argus_light_2d_get_handle(self.ffi_handle).into() }
    }

    pub fn get_type(&self) -> Light2dType {
        unsafe { Light2dType::unchecked_transmute_from(argus_light_2d_get_type(self.ffi_handle)) }
    }

    pub fn is_occludable(&self) -> bool {
        unsafe { argus_light_2d_is_occludable(self.ffi_handle)}
    }

    pub fn get_color(&self) -> Vector3f {
        unsafe { argus_light_2d_get_color(self.ffi_handle).into() }
    }

    pub fn set_color(&mut self, color: Vector3f) {
        unsafe { argus_light_2d_set_color(self.ffi_handle, color.into()); }
    }

    pub fn get_parameters(&self) -> Light2dParameters {
        unsafe { argus_light_2d_get_parameters(self.ffi_handle).into() }
    }

    pub fn set_parameters(&mut self, params: Light2dParameters) {
        unsafe { argus_light_2d_set_parameters(self.ffi_handle, params.into()); }
    }

    pub fn get_transform(&self) -> Transform2d {
        unsafe { argus_light_2d_get_transform(self.ffi_handle).into() }
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        unsafe { argus_light_2d_set_transform(self.ffi_handle, transform.into()); }
    }
}

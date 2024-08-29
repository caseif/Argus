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

use argus_macros::ffi_repr;

use crate::lowlevel_cabi::*;

#[repr(C)]
#[ffi_repr(argus_vector_2d_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd)]
pub struct Vector2d {
    pub x: f64,
    pub y: f64,
}

#[repr(C)]
#[ffi_repr(argus_vector_3d_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd)]
pub struct Vector3d {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[repr(C)]
#[ffi_repr(argus_vector_4d_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd)]
pub struct Vector4d {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

#[repr(C)]
#[ffi_repr(argus_vector_2f_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd)]
pub struct Vector2f {
    pub x: f32,
    pub y: f32,
}

#[repr(C)]
#[ffi_repr(argus_vector_3f_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd)]
pub struct Vector3f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

#[repr(C)]
#[ffi_repr(argus_vector_4f_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd)]
pub struct Vector4f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub w: f32,
}

#[repr(C)]
#[ffi_repr(argus_vector_2i_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Vector2i {
    pub x: i32,
    pub y: i32,
}

#[repr(C)]
#[ffi_repr(argus_vector_3i_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Vector3i {
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

#[repr(C)]
#[ffi_repr(argus_vector_4i_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Vector4i {
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub w: i32,
}

#[repr(C)]
#[ffi_repr(argus_vector_2u_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Vector2u {
    pub x: u32,
    pub y: u32,
}

#[repr(C)]
#[ffi_repr(argus_vector_3u_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Vector3u {
    pub x: u32,
    pub y: u32,
    pub z: u32,
}

#[repr(C)]
#[ffi_repr(argus_vector_4u_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Vector4u {
    pub x: u32,
    pub y: u32,
    pub z: u32,
    pub w: u32,
}

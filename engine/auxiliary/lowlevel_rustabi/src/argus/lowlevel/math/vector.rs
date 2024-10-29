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
use serde::{Deserialize, Serialize};
use crate::lowlevel_cabi::*;

macro_rules! create_vector2_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl std::ops::Add<Self> for $vec_type {
            type Output = Self;

            fn add(self, rhs: Self) -> Self::Output { Self { x: self.x + rhs.x, y: self.y + rhs.y } }
        }

        impl std::ops::Sub<Self> for $vec_type {
            type Output = Self;

            fn sub(self, rhs: Self) -> Self::Output { Self { x: self.x - rhs.x, y: self.y - rhs.y } }
        }

        impl std::ops::Mul<$el_type> for $vec_type {
            type Output = Self;

            fn mul(self, rhs: $el_type) -> Self::Output { Self { x: self.x * rhs, y: self.y * rhs } }
        }

        impl std::ops::Div<$el_type> for $vec_type {
            type Output = Self;

            fn div(self, rhs: $el_type) -> Self::Output { Self { x: self.x / rhs, y: self.y / rhs } }
        }
    }
}

macro_rules! create_vector3_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl std::ops::Add<Self> for $vec_type {
            type Output = Self;

            fn add(self, rhs: Self) -> Self::Output { Self { x: self.x + rhs.x, y: self.y + rhs.y, z: self.z + rhs.z } }
        }

        impl std::ops::Sub<Self> for $vec_type {
            type Output = Self;

            fn sub(self, rhs: Self) -> Self::Output { Self { x: self.x - rhs.x, y: self.y - rhs.y, z: self.z - rhs.z } }
        }

        impl std::ops::Mul<$el_type> for $vec_type {
            type Output = Self;

            fn mul(self, rhs: $el_type) -> Self::Output { Self { x: self.x * rhs, y: self.y * rhs, z: self.z * rhs } }
        }

        impl std::ops::Div<$el_type> for $vec_type {
            type Output = Self;

            fn div(self, rhs: $el_type) -> Self::Output { Self { x: self.x / rhs, y: self.y / rhs, z: self.z / rhs } }
        }
    }
}

macro_rules! create_vector4_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl std::ops::Add<Self> for $vec_type {
            type Output = Self;

            fn add(self, rhs: Self) -> Self::Output {
                Self {
                    x: self.x + rhs.x,
                    y: self.y + rhs.y,
                    z: self.z + rhs.z,
                    w: self.w + rhs.w,
                }
            }
        }

        impl std::ops::Sub<Self> for $vec_type {
            type Output = Self;

            fn sub(self, rhs: Self) -> Self::Output {
                Self {
                    x: self.x - rhs.x,
                    y: self.y - rhs.y,
                    z: self.z - rhs.z,
                    w: self.w - rhs.w,
                }
            }
        }

        impl std::ops::Mul<$el_type> for $vec_type {
            type Output = Self;

            fn mul(self, rhs: $el_type) -> Self::Output {
                Self {
                    x: self.x * rhs,
                    y: self.y * rhs,
                    z: self.z * rhs,
                    w: self.w * rhs,
                }
            }
        }

        impl std::ops::Div<$el_type> for $vec_type {
            type Output = Self;

            fn div(self, rhs: $el_type) -> Self::Output {
                Self {
                    x: self.x / rhs,
                    y: self.y / rhs,
                    z: self.z / rhs,
                    w: self.w / rhs,
                }
            }
        }
    }
}

#[repr(C)]
#[ffi_repr(argus_vector_2d_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector2d {
    pub x: f64,
    pub y: f64,
}

impl Vector2d {
    pub fn new(x: f64, y: f64) -> Self { Self { x, y } }
}

create_vector2_ops!(Vector2d, f64);

#[repr(C)]
#[ffi_repr(argus_vector_3d_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector3d {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Vector3d {
    pub fn new(x: f64, y: f64, z: f64) -> Self { Self { x, y, z } }
}

create_vector3_ops!(Vector3d, f64);

#[repr(C)]
#[ffi_repr(argus_vector_4d_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector4d {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

impl Vector4d {
    pub fn new(x: f64, y: f64, z: f64, w: f64) -> Self { Self { x, y, z, w } }
}

create_vector4_ops!(Vector4d, f64);

#[repr(C)]
#[ffi_repr(argus_vector_2f_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector2f {
    pub x: f32,
    pub y: f32,
}

impl Vector2f {
    pub fn new(x: f32, y: f32) -> Self { Self { x, y } }
}

create_vector2_ops!(Vector2f, f32);

#[repr(C)]
#[ffi_repr(argus_vector_3f_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector3f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

impl Vector3f {
    pub fn new(x: f32, y: f32, z: f32) -> Self { Self { x, y, z } }
}

create_vector3_ops!(Vector3f, f32);

#[repr(C)]
#[ffi_repr(argus_vector_4f_t)]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector4f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub w: f32,
}

impl Vector4f {
    pub fn new(x: f32, y: f32, z: f32, w: f32) -> Self { Self { x, y, z, w } }
}

create_vector4_ops!(Vector4f, f32);

#[repr(C)]
#[ffi_repr(argus_vector_2i_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector2i {
    pub x: i32,
    pub y: i32,
}

impl Vector2i {
    pub fn new(x: i32, y: i32) -> Self { Self { x, y } }
}

create_vector2_ops!(Vector2i, i32);

#[repr(C)]
#[ffi_repr(argus_vector_3i_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector3i {
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

impl Vector3i {
    pub fn new(x: i32, y: i32, z: i32) -> Self { Self { x, y, z } }
}

create_vector3_ops!(Vector3i, i32);

#[repr(C)]
#[ffi_repr(argus_vector_4i_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector4i {
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub w: i32,
}

impl Vector4i {
    pub fn new(x: i32, y: i32, z: i32, w: i32) -> Self { Self { x, y, z, w } }
}

create_vector4_ops!(Vector4i, i32);

#[repr(C)]
#[ffi_repr(argus_vector_2u_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector2u {
    pub x: u32,
    pub y: u32,
}

impl Vector2u {
    pub fn new(x: u32, y: u32) -> Self { Self { x, y } }
}

create_vector2_ops!(Vector2u, u32);

#[repr(C)]
#[ffi_repr(argus_vector_3u_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector3u {
    pub x: u32,
    pub y: u32,
    pub z: u32,
}

impl Vector3u {
    pub fn new(x: u32, y: u32, z: u32) -> Self { Self { x, y, z } }
}

create_vector3_ops!(Vector3u, u32);

#[repr(C)]
#[ffi_repr(argus_vector_4u_t)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector4u {
    pub x: u32,
    pub y: u32,
    pub z: u32,
    pub w: u32,
}

impl Vector4u {
    pub fn new(x: u32, y: u32, z: u32, w: u32) -> Self { Self { x, y, z, w } }
}

create_vector4_ops!(Vector4u, u32);

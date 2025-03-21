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

use argus_scripting_bind::script_bind;
use serde::{Deserialize, Serialize};

macro_rules! create_vector2_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl Into<[$el_type; 2]> for $vec_type {
            fn into(self) -> [$el_type; 2] {
                [self.x, self.y]
            }
        }

        impl std::ops::Add<Self> for $vec_type {
            type Output = Self;

            fn add(self, rhs: Self) -> Self::Output { Self { x: self.x + rhs.x, y: self.y + rhs.y } }
        }

        impl std::ops::Add<($el_type, $el_type)> for $vec_type {
            type Output = Self;

            fn add(self, rhs: ($el_type, $el_type)) -> Self::Output { Self { x: self.x + rhs.0, y: self.y + rhs.1 } }
        }

        impl std::ops::Sub<Self> for $vec_type {
            type Output = Self;

            fn sub(self, rhs: Self) -> Self::Output { Self { x: self.x - rhs.x, y: self.y - rhs.y } }
        }

        impl std::ops::Sub<($el_type, $el_type)> for $vec_type {
            type Output = Self;

            fn sub(self, rhs: ($el_type, $el_type)) -> Self::Output { Self { x: self.x - rhs.0, y: self.y - rhs.1 } }
        }

        impl std::ops::Mul<$el_type> for $vec_type {
            type Output = Self;

            fn mul(self, rhs: $el_type) -> Self::Output { Self { x: self.x * rhs, y: self.y * rhs } }
        }

        impl std::ops::Mul<$vec_type> for $vec_type {
            type Output = Self;

            fn mul(self, rhs: $vec_type) -> Self::Output { Self { x: self.x * rhs.x, y: self.y * rhs.y } }
        }

        impl std::ops::Div<$el_type> for $vec_type {
            type Output = Self;

            fn div(self, rhs: $el_type) -> Self::Output { Self { x: self.x / rhs, y: self.y / rhs } }
        }

        impl std::ops::AddAssign<Self> for $vec_type {
            fn add_assign(&mut self, rhs: Self) {
                self.x += rhs.x;
                self.y += rhs.y;
            }
        }

        impl std::ops::AddAssign<($el_type, $el_type)> for $vec_type {
            fn add_assign(&mut self, rhs: ($el_type, $el_type)) {
                self.x += rhs.0;
                self.y += rhs.1;
            }
        }

        impl std::ops::SubAssign<Self> for $vec_type {
            fn sub_assign(&mut self, rhs: Self) {
                self.x -= rhs.x;
                self.y -= rhs.y;
            }
        }

        impl std::ops::SubAssign<($el_type, $el_type)> for $vec_type {
            fn sub_assign(&mut self, rhs: ($el_type, $el_type)) {
                self.x -= rhs.0;
                self.y -= rhs.1;
            }
        }

        impl std::ops::MulAssign<$el_type> for $vec_type {
            fn mul_assign(&mut self, rhs: $el_type) {
                self.x *= rhs;
                self.y *= rhs;
            }
        }

        impl std::ops::DivAssign<$el_type> for $vec_type {
            fn div_assign(&mut self, rhs: $el_type) {
                self.x /= rhs;
                self.y /= rhs;
            }
        }

        impl $vec_type {
            #[inline(always)]
            pub fn is_zero(&self) -> bool {
                self.x == (0 as $el_type) && self.y == (0 as $el_type)
            }

            #[inline(always)]
            pub fn abs(&self) -> Self {
                Self {
                    x: if self.x < 0 as $el_type { self.x * -1i32 as $el_type } else { self.x },
                    y: if self.y < 0 as $el_type { self.y * -1i32 as $el_type } else { self.y },
                }
            }

            #[inline(always)]
            pub fn dist(&self, other: &$vec_type) -> f32 {
                self.dist_squared(other).sqrt()
            }

            #[inline(always)]
            pub fn dist_squared(&self, other: &$vec_type) -> f32 {
                let dx = if self.x > other.x { self.x - other.x } else { other.x - self.x };
                let dy = if self.y > other.y { self.y - other.y } else { other.y - self.y };
                (dx * dx + dy * dy) as f32
            }

            #[inline(always)]
            pub fn dist_manhattan(&self, other: &$vec_type) -> $el_type {
                let dx = if self.x > other.x { self.x - other.x } else { other.x - self.x };
                let dy = if self.y > other.y { self.y - other.y } else { other.y - self.y };
                dx + dy
            }

            #[inline(always)]
            pub fn mag(&self) -> f32 {
                self.mag_squared().sqrt()
            }

            #[inline(always)]
            pub fn mag_squared(&self) -> f32 {
                (self.x * self.x + self.y * self.y) as f32
            }

            #[inline(always)]
            pub fn cross_mag(&self, other: &$vec_type) -> $el_type {
                self.x * other.y - self.y * other.x
            }

            #[inline(always)]
            pub fn dot(&self, other: &$vec_type) -> $el_type {
                self.x * other.x + self.y * other.y
            }
        }
    }
}

macro_rules! create_vector2_signed_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl std::ops::Neg for $vec_type {
            type Output = Self;
            fn neg(self) -> Self::Output {
                Self { x: -self.x, y: -self.y }
            }
        }
    }
}

macro_rules! create_vector3_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl Into<[$el_type; 3]> for $vec_type {
            fn into(self) -> [$el_type; 3] {
                [self.x, self.y, self.z]
            }
        }

        impl std::ops::Add<Self> for $vec_type {
            type Output = Self;

            fn add(self, rhs: Self) -> Self::Output {
                Self { x: self.x + rhs.x, y: self.y + rhs.y, z: self.z + rhs.z }
            }
        }

        impl std::ops::Add<($el_type, $el_type, $el_type)> for $vec_type {
            type Output = Self;

            fn add(self, rhs: ($el_type, $el_type, $el_type)) -> Self::Output {
                Self { x: self.x + rhs.0, y: self.y + rhs.1, z: self.z + rhs.2 }
            }
        }

        impl std::ops::Sub<Self> for $vec_type {
            type Output = Self;

            fn sub(self, rhs: Self) -> Self::Output {
                Self { x: self.x - rhs.x, y: self.y - rhs.y, z: self.z - rhs.z }
            }
        }

        impl std::ops::Sub<($el_type, $el_type, $el_type)> for $vec_type {
            type Output = Self;

            fn sub(self, rhs: ($el_type, $el_type, $el_type)) -> Self::Output {
                Self { x: self.x - rhs.0, y: self.y - rhs.1, z: self.z - rhs.2 }
            }
        }

        impl std::ops::Mul<$el_type> for $vec_type {
            type Output = Self;

            fn mul(self, rhs: $el_type) -> Self::Output { Self { x: self.x * rhs, y: self.y * rhs, z: self.z * rhs } }
        }

        impl std::ops::Mul<$vec_type> for $vec_type {
            type Output = Self;

            fn mul(self, rhs: $vec_type) -> Self::Output { Self { x: self.x * rhs.x, y: self.y * rhs.y, z: self.z * rhs.z } }
        }

        impl std::ops::Div<$el_type> for $vec_type {
            type Output = Self;

            fn div(self, rhs: $el_type) -> Self::Output { Self { x: self.x / rhs, y: self.y / rhs, z: self.z / rhs } }
        }

        impl std::ops::AddAssign<Self> for $vec_type {
            fn add_assign(&mut self, rhs: Self) {
                self.x += rhs.x;
                self.y += rhs.y;
                self.z += rhs.z;
            }
        }

        impl std::ops::AddAssign<($el_type, $el_type, $el_type)> for $vec_type {
            fn add_assign(&mut self, rhs: ($el_type, $el_type, $el_type)) {
                self.x += rhs.0;
                self.y += rhs.1;
                self.z += rhs.2;
            }
        }

        impl std::ops::SubAssign<Self> for $vec_type {
            fn sub_assign(&mut self, rhs: Self) {
                self.x -= rhs.x;
                self.y -= rhs.y;
                self.z -= rhs.z;
            }
        }

        impl std::ops::SubAssign<($el_type, $el_type, $el_type)> for $vec_type {
            fn sub_assign(&mut self, rhs: ($el_type, $el_type, $el_type)) {
                self.x -= rhs.0;
                self.y -= rhs.1;
                self.z -= rhs.2;
            }
        }

        impl std::ops::MulAssign<$el_type> for $vec_type {
            fn mul_assign(&mut self, rhs: $el_type) {
                self.x *= rhs;
                self.y *= rhs;
                self.z *= rhs;
            }
        }

        impl std::ops::DivAssign<$el_type> for $vec_type {
            fn div_assign(&mut self, rhs: $el_type) {
                self.x /= rhs;
                self.y /= rhs;
                self.z /= rhs;
            }
        }

        impl $vec_type {
            pub fn mag(&self) -> f32 {
                return ((self.x * self.x + self.y * self.y + self.z * self.z) as f32).sqrt();
            }
        }
    }
}

macro_rules! create_vector3_signed_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl std::ops::Neg for $vec_type {
            type Output = Self;
            fn neg(self) -> Self::Output {
                Self { x: -self.x, y: -self.y, z: -self.z }
            }
        }
    }
}

macro_rules! create_vector4_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl Into<[$el_type; 4]> for $vec_type {
            fn into(self) -> [$el_type; 4] {
                [self.x, self.y, self.z, self.w]
            }
        }

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

        impl std::ops::Add<($el_type, $el_type, $el_type, $el_type)> for $vec_type {
            type Output = Self;

            fn add(self, rhs: ($el_type, $el_type, $el_type, $el_type)) -> Self::Output {
                Self {
                    x: self.x + rhs.0,
                    y: self.y + rhs.1,
                    z: self.z + rhs.2,
                    w: self.w + rhs.3,
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

        impl std::ops::Sub<($el_type, $el_type, $el_type, $el_type)> for $vec_type {
            type Output = Self;

            fn sub(self, rhs: ($el_type, $el_type, $el_type, $el_type)) -> Self::Output {
                Self {
                    x: self.x - rhs.0,
                    y: self.y - rhs.1,
                    z: self.z - rhs.2,
                    w: self.w - rhs.3,
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

        impl std::ops::Mul<$vec_type> for $vec_type {
            type Output = Self;

            fn mul(self, rhs: $vec_type) -> Self::Output {
                Self {
                    x: self.x * rhs.x,
                    y: self.y * rhs.y,
                    z: self.z * rhs.z,
                    w: self.w * rhs.w,
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

        impl std::ops::AddAssign<Self> for $vec_type {
            fn add_assign(&mut self, rhs: Self) {
                self.x += rhs.x;
                self.y += rhs.y;
                self.z += rhs.z;
                self.w += rhs.w;
            }
        }

        impl std::ops::AddAssign<($el_type, $el_type, $el_type, $el_type)> for $vec_type {
            fn add_assign(&mut self, rhs: ($el_type, $el_type, $el_type, $el_type)) {
                self.x += rhs.0;
                self.y += rhs.1;
                self.z += rhs.2;
                self.w += rhs.3;
            }
        }

        impl std::ops::SubAssign<Self> for $vec_type {
            fn sub_assign(&mut self, rhs: Self) {
                self.x -= rhs.x;
                self.y -= rhs.y;
                self.z -= rhs.z;
                self.w -= rhs.w;
            }
        }

        impl std::ops::SubAssign<($el_type, $el_type, $el_type, $el_type)> for $vec_type {
            fn sub_assign(&mut self, rhs: ($el_type, $el_type, $el_type, $el_type)) {
                self.x -= rhs.0;
                self.y -= rhs.1;
                self.z -= rhs.2;
                self.w -= rhs.3;
            }
        }

        impl std::ops::MulAssign<$el_type> for $vec_type {
            fn mul_assign(&mut self, rhs: $el_type) {
                self.x *= rhs;
                self.y *= rhs;
                self.z *= rhs;
                self.w *= rhs;
            }
        }

        impl std::ops::DivAssign<$el_type> for $vec_type {
            fn div_assign(&mut self, rhs: $el_type) {
                self.x /= rhs;
                self.y /= rhs;
                self.z /= rhs;
                self.w /= rhs;
            }
        }

        impl $vec_type {
            pub fn mag(&self) -> f32 {
                return (
                    (self.x * self.x + self.y * self.y + self.z * self.z + self.w * self.w) as f32
                )
                    .sqrt();
            }
        }
    }
}

macro_rules! create_vector4_signed_ops {
    ($vec_type: ty, $el_type: ty) => {
        impl std::ops::Neg for $vec_type {
            type Output = Self;
            fn neg(self) -> Self::Output {
                Self { x: -self.x, y: -self.y, z: -self.z, w: -self.w }
            }
        }
    }
}

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector2d {
    pub x: f64,
    pub y: f64,
}

#[script_bind]
impl Vector2d {
    #[script_bind]
    pub fn new(x: f64, y: f64) -> Self { Self { x, y } }
}

create_vector2_ops!(Vector2d, f64);
create_vector2_signed_ops!(Vector2d, f64);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector3d {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[script_bind]
impl Vector3d {
    #[script_bind]
    pub fn new(x: f64, y: f64, z: f64) -> Self { Self { x, y, z } }
}

create_vector3_ops!(Vector3d, f64);
create_vector3_signed_ops!(Vector3d, f64);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector4d {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

#[script_bind]
impl Vector4d {
    #[script_bind]
    pub fn new(x: f64, y: f64, z: f64, w: f64) -> Self { Self { x, y, z, w } }
}

create_vector4_ops!(Vector4d, f64);
create_vector4_signed_ops!(Vector4d, f64);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector2f {
    pub x: f32,
    pub y: f32,
}

#[script_bind]
impl Vector2f {
    #[script_bind]
    pub fn new(x: f32, y: f32) -> Self { Self { x, y } }
}

create_vector2_ops!(Vector2f, f32);
create_vector2_signed_ops!(Vector2f, f32);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector3f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

#[script_bind]
impl Vector3f {
    #[script_bind]
    pub fn new(x: f32, y: f32, z: f32) -> Self { Self { x, y, z } }
}

create_vector3_ops!(Vector3f, f32);
create_vector3_signed_ops!(Vector3f, f32);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, PartialEq, PartialOrd, Deserialize, Serialize)]
pub struct Vector4f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub w: f32,
}

#[script_bind]
impl Vector4f {
    #[script_bind]
    pub fn new(x: f32, y: f32, z: f32, w: f32) -> Self { Self { x, y, z, w } }
}

create_vector4_ops!(Vector4f, f32);
create_vector4_signed_ops!(Vector4f, f32);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector2i {
    pub x: i32,
    pub y: i32,
}

#[script_bind]
impl Vector2i {
    #[script_bind]
    pub fn new(x: i32, y: i32) -> Self { Self { x, y } }
}

create_vector2_ops!(Vector2i, i32);
create_vector2_signed_ops!(Vector2i, i32);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector3i {
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

#[script_bind]
impl Vector3i {
    #[script_bind]
    pub fn new(x: i32, y: i32, z: i32) -> Self { Self { x, y, z } }
}

create_vector3_ops!(Vector3i, i32);
create_vector3_signed_ops!(Vector3i, i32);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector4i {
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub w: i32,
}

#[script_bind]
impl Vector4i {
    #[script_bind]
    pub fn new(x: i32, y: i32, z: i32, w: i32) -> Self { Self { x, y, z, w } }
}

create_vector4_ops!(Vector4i, i32);
create_vector4_signed_ops!(Vector4i, i32);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector2u {
    pub x: u32,
    pub y: u32,
}

#[script_bind]
impl Vector2u {
    #[script_bind]
    pub fn new(x: u32, y: u32) -> Self { Self { x, y } }
}

create_vector2_ops!(Vector2u, u32);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector3u {
    pub x: u32,
    pub y: u32,
    pub z: u32,
}

#[script_bind]
impl Vector3u {
    #[script_bind]
    pub fn new(x: u32, y: u32, z: u32) -> Self { Self { x, y, z } }
}

create_vector3_ops!(Vector3u, u32);

#[repr(C)]
#[script_bind]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct Vector4u {
    pub x: u32,
    pub y: u32,
    pub z: u32,
    pub w: u32,
}

#[script_bind]
impl Vector4u {
    #[script_bind]
    pub fn new(x: u32, y: u32, z: u32, w: u32) -> Self { Self { x, y, z, w } }
}

create_vector4_ops!(Vector4u, u32);

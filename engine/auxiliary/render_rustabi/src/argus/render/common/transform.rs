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
use argus_macros::ffi_repr;
use lowlevel_rustabi::argus::lowlevel::{Vector2f, Vector4f};

use crate::render_cabi::*;

#[repr(C)]
#[ffi_repr(ArgusTransform2d)]
#[derive(Clone, Copy, Debug, Default)]
pub struct Transform2d {
    pub translation: Vector2f,
    pub scale: Vector2f,
    pub rotation: f32,
}

impl Transform2d {
    pub fn new(translation: Vector2f, scale: Vector2f, rotation: f32) -> Self {
        Self {
            translation,
            scale,
            rotation,
        }
    }
}

#[repr(C)]
#[ffi_repr(argus_matrix_4x4_t)]
#[derive(Clone, Copy, Debug, Default)]
pub struct Matrix4x4 {
    pub cells: [f32; 16],
}

impl Transform2d {
    fn as_ptr(&self) -> *const ArgusTransform2d {
        ptr::from_ref(self.into())
    }

    pub fn add_translation(&mut self, x: f32, y: f32) {
        self.translation += (x, y);
    }

    pub fn add_rotation(&mut self, rads: f32) {
        self.rotation += rads;
    }

    pub fn as_matrix(&self, anchor_x: f32, anchor_y: f32) -> Matrix4x4 {
        unsafe { argus_transform_2d_as_matrix(self.as_ptr(), anchor_x, anchor_y).into() }
    }

    pub fn get_translation_matrix(&self) -> Matrix4x4 {
        unsafe { argus_transform_2d_get_translation_matrix(self.as_ptr()).into() }
    }

    pub fn get_rotation_matrix(&self) -> Matrix4x4 {
        unsafe { argus_transform_2d_get_rotation_matrix(self.as_ptr()).into() }
    }

    pub fn get_scale_matrix(&self) -> Matrix4x4 {
        unsafe { argus_transform_2d_get_scale_matrix(self.as_ptr()).into() }
    }

    pub fn inverse(&self) -> Transform2d {
        unsafe { argus_transform_2d_inverse(self.as_ptr()).into() }
    }
}

impl Matrix4x4 {
    pub fn from_row_major(vals: [f32; 16]) -> Self {
        Self { cells: [
            vals[0], vals[4], vals[8], vals[12],
            vals[1], vals[5], vals[9], vals[13],
            vals[2], vals[6], vals[10], vals[14],
            vals[3], vals[7], vals[11], vals[15],
        ] }
    }

    pub fn get(&self, row: usize, col: usize) -> f32 {
        self.cells[col * 4 + row]
    }

    pub fn get_mut(&mut self, row: usize, col: usize) -> &mut f32 {
        &mut self.cells[col * 4 + row]
    }

    pub fn multiply_matrix(&self, other: Self) -> Self {
        let mut res: [f32; 16] = Default::default();

        // naive implementation
        for i in 0..4 {
            for j in 0..4 {
                res[j * 4 + i] = 0.0;
                for k in 0..4 {
                    res[j * 4 + i] += self.get(i, k) * other.get(k, j);
                }
            }
        }

        Self { cells: res }
    }

    pub fn multiply_vector(&self, vector: Vector4f) -> Vector4f {
        Vector4f {
            x: self.get(0, 0) * vector.x +
                self.get(0, 1) * vector.y +
                self.get(0, 2) * vector.z +
                self.get(0, 3) * vector.w,
            y: self.get(1, 0) * vector.x +
                self.get(1, 1) * vector.y +
                self.get(1, 2) * vector.z +
                self.get(1, 3) * vector.w,
            z: self.get(2, 0) * vector.x +
                self.get(2, 1) * vector.y +
                self.get(2, 2) * vector.z +
                self.get(2, 3) * vector.w,
            w: self.get(3, 0) * vector.x +
                self.get(3, 1) * vector.y +
                self.get(3, 2) * vector.z +
                self.get(3, 3) * vector.w
        }
    }
}

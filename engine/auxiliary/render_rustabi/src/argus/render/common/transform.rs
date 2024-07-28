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
use lowlevel_rustabi::argus::lowlevel::Vector2f;

use crate::render_cabi::*;

#[repr(C)]
#[ffi_repr(ArgusTransform2d)]
#[derive(Clone, Copy, Debug, Default)]
pub struct Transform2d {
    translation: Vector2f,
    scale: Vector2f,
    rotation: f32,
}

#[repr(C)]
#[ffi_repr(argus_matrix_4x4_t)]
#[derive(Clone, Copy, Debug, Default)]
pub struct Matrix4x4 {
    cells: [f32; 16],
}

impl Transform2d {
    fn as_ptr(&self) -> *const ArgusTransform2d {
        ptr::from_ref(self.into())
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
    pub fn at(&self, row: usize, col: usize) -> f32 {
        self.cells[col * 4 + row]
    }
}

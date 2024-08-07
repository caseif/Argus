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
use lowlevel_rustabi::argus::lowlevel::{Vector2f, Vector3f, Vector4f};

use crate::render_cabi::*;

#[repr(C)]
#[ffi_repr(ArgusVertex2d)]
#[derive(Clone, Copy, Debug)]
pub struct Vertex2d {
    pub position: Vector2f,
    pub normal: Vector2f,
    pub color: Vector4f,
    pub tex_coord: Vector2f,
}
#[repr(C)]
#[ffi_repr(ArgusVertex3d)]
#[derive(Clone, Copy, Debug)]
pub struct Vertex3d {
    pub position: Vector3f,
    pub normal: Vector3f,
    pub color: Vector4f,
    pub tex_coord: Vector2f,
}

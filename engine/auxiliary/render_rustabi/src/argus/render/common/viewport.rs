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
use lowlevel_rustabi::lowlevel_cabi::argus_vector_2f_t;

use crate::render_cabi::*;

#[repr(u32)]
#[derive(Debug, Clone, Copy, Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, UnsafeFromPrimitive)]
pub enum ViewportCoordinateSpaceMode {
    Individual = ARGUS_VCSM_INDIVIDUAL,
    MinAxis = ARGUS_VCSM_MIN_AXIS,
    MaxAxis = ARGUS_VCSM_MAX_AXIS,
    HorizontalAxis = ARGUS_VCSM_HORIZONTAL_AXIS,
    VerticalAxis = ARGUS_VCSM_VERTICAL_AXIS,
}

#[repr(C)]
#[ffi_repr(ArgusViewport)]
#[derive(Clone, Copy, Debug)]
pub struct Viewport {
    pub top: f32,
    pub bottom: f32,
    pub left: f32,
    pub right: f32,
    pub scaling: argus_vector_2f_t,
    pub mode: ViewportCoordinateSpaceMode,
}

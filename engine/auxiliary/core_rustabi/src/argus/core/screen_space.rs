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

use num_enum::{IntoPrimitive, TryFromPrimitive};

use crate::core_cabi;

use core_cabi::ScreenSpace;

#[derive(Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, TryFromPrimitive)]
#[repr(u32)]
pub enum ScreenSpaceScaleMode {
    NormalizeMinDim = core_cabi::SSS_MODE_NORMALIZE_MIN_DIM,
    NormalizeMaxDim = core_cabi::SSS_MODE_NORMALIZE_MAX_DIM,
    NormalizeVertical = core_cabi::SSS_MODE_NORMALIZE_VERTICAL,
    NormalizeHorizontal = core_cabi::SSS_MODE_NORMALIZE_HORIZONTAL,
    None = core_cabi::SSS_MODE_NONE,
}

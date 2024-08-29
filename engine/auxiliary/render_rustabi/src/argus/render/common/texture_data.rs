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

use std::slice;
use lowlevel_rustabi::argus::lowlevel::FfiWrapper;
use crate::render_cabi::*;

pub struct TextureData {
    handle: argus_texture_data_t,
}

impl FfiWrapper for TextureData {
    fn of(handle: argus_texture_data_t) -> Self {
        Self { handle }
    }
}

impl TextureData {
    pub fn get_width(&self) -> u32 {
        unsafe { argus_texture_data_get_width(self.handle) }
    }

    pub fn get_height(&self) -> u32 {
        unsafe { argus_texture_data_get_height(self.handle) }
    }

    pub fn get_pixel_data(&self)
        -> Vec<&[u8]> {
        unsafe {
            let width = self.get_width() as usize;
            let height = self.get_height() as usize;
            let ptr = argus_texture_data_get_pixel_data(self.handle);

            slice::from_raw_parts(ptr, height)
                .into_iter()
                .map(|row| slice::from_raw_parts(*row, width))
                .collect()
        }
    }
}

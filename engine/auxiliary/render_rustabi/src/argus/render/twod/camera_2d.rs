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

use lowlevel_rustabi::argus::lowlevel::ValueAndDirtyFlag;
use lowlevel_rustabi::util::cstr_to_str;

use crate::argus::render::{Scene2d, Transform2d};
use crate::render_cabi::*;

pub struct Camera2d {
    handle: argus_camera_2d_t,
}

impl Camera2d {
    pub(crate) fn of(handle: argus_camera_2d_t) -> Self {
        Self { handle }
    }

    pub(crate) fn get_handle(&self) -> argus_camera_2d_t {
        self.handle
    }

    pub fn get_id(&self) -> &str {
        unsafe { cstr_to_str(argus_camera_2d_get_id(self.handle)) }
    }

    pub fn get_scene(&self) -> Scene2d {
        unsafe { Scene2d::of(argus_camera_2d_get_scene(self.handle)) }
    }

    pub fn peek_transform(&self) -> Transform2d {
        unsafe {
            argus_camera_2d_peek_transform(self.handle).into()
        }
    }

    pub fn get_transform(&mut self) -> ValueAndDirtyFlag<Transform2d> {
        unsafe {
            let mut dirty = false;
            ValueAndDirtyFlag::of(
                argus_camera_2d_get_transform(self.handle, &mut dirty).into(),
                dirty,
            )
        }
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        unsafe { argus_camera_2d_set_transform(self.handle, transform.into()) }
    }
}

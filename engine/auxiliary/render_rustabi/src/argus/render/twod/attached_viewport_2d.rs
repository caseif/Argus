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
use crate::argus::render::{AttachedViewport, Camera2d};
use crate::render_cabi::*;

pub struct AttachedViewport2d {
    handle: argus_attached_viewport_2d_t,
}

impl AttachedViewport2d {
    pub(crate) fn of(handle: argus_attached_viewport_2d_t) -> Self {
        Self { handle }
    }
    
    pub fn get_camera(&self) -> Camera2d {
        unsafe { Camera2d::of(argus_attached_viewport_2d_get_camera(self.handle)) }
    }
}

impl Into<AttachedViewport> for AttachedViewport2d {
    fn into(self) -> AttachedViewport {
        AttachedViewport::of(self.handle)
    }
}

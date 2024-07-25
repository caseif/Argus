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
use crate::argus::render::Scene2d;
use crate::render_cabi::*;
use lowlevel_rustabi::util::str_to_cstring;
use num_enum::{IntoPrimitive, UnsafeFromPrimitive};

#[repr(u32)]
#[derive(Debug, Clone, Eq, Ord, PartialEq, PartialOrd, IntoPrimitive, UnsafeFromPrimitive)]
pub enum SceneType {
    TwoD = ARGUS_SCENE_TYPE_TWO_D,
    ThreeD = ARGUS_SCENE_TYPE_THREE_D,
}

pub struct Scene {
    handle: argus_scene_t,
}

impl Scene {
    pub(crate) fn of(handle: argus_scene_t) -> Self {
        Self { handle }
    }

    pub fn as_2d(&self) -> Scene2d {
        Scene2d::of(self.handle)
    }

    pub fn find(id: &str) -> Option<Self> {
        unsafe {
            let id_c = str_to_cstring(id);
            argus_scene_find(id_c.as_ptr()).as_mut().map(|handle| Scene::of(handle))
        }
    }

    pub fn get_type(&self) -> SceneType {
        unsafe { SceneType::unchecked_transmute_from(argus_scene_get_type(self.handle)) }
    }
}

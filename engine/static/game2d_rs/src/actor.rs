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
use lowlevel_rustabi::argus::lowlevel::{Dirtiable, Handle, Vector2f};
use render_rustabi::argus::render::Transform2d;
use resman_rustabi::argus::resman::Resource;
use crate::sprite::Sprite;

pub struct Actor2d {
    size: Vector2f,
    z_index: u32,
    pub(crate) can_occlude_light: Dirtiable<bool>,
    pub(crate) transform: Dirtiable<Transform2d>,

    sprite: Sprite,

    pub(crate) render_obj: Option<Handle>,
}

impl Actor2d {
    pub fn new(
        sprite_defn_res: Resource,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        transform: Transform2d,
    ) -> Actor2d {
        let sprite = Sprite::new(sprite_defn_res);

        Self {
            size,
            z_index,
            can_occlude_light: Dirtiable::new(can_occlude_light),
            transform: Dirtiable::new(transform),
            sprite,
            render_obj: None,
        }
    }

    pub fn get_size(&self) -> Vector2f {
        self.size
    }

    pub fn get_z_index(&self) -> u32 {
        self.z_index
    }

    pub fn can_occlude_light(&self) -> bool {
        self.can_occlude_light.peek().value
    }

    pub fn set_can_occlude_light(&mut self, can_occlude: bool) {
        self.can_occlude_light.set(can_occlude);
    }

    pub fn get_transform(&self) -> Transform2d {
        self.transform.peek().value
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        self.transform.set(transform);
    }

    pub fn get_sprite(&self) -> &Sprite {
        &self.sprite
    }

    pub fn get_sprite_mut(&mut self) -> &mut Sprite {
        &mut self.sprite
    }
}

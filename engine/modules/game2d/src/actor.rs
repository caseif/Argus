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
use argus_scripting_bind::script_bind;
use argus_resman::Resource;
use argus_render::common::Transform2d;
use argus_util::dirtiable::Dirtiable;
use argus_util::math::Vector2f;
use argus_util::pool::Handle;
use crate::sprite::Sprite;

#[script_bind(ref_only)]
pub struct Actor2d {
    size: Vector2f,
    z_index: u32,
    pub(crate) velocity: Vector2f,
    pub(crate) can_occlude_light: Dirtiable<bool>,
    pub(crate) transform: Dirtiable<Transform2d>,

    sprite: Sprite,

    pub(crate) render_obj: Option<Handle>,
}

#[script_bind]
impl Actor2d {
    pub(crate) fn new(
        sprite_defn_res: Resource,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
    ) -> Actor2d {
        let sprite = Sprite::new(sprite_defn_res);

        Self {
            size,
            z_index,
            velocity: Vector2f::default(),
            can_occlude_light: Dirtiable::new(can_occlude_light),
            transform: Default::default(),
            sprite,
            render_obj: None,
        }
    }

    #[script_bind]
    pub fn get_size(&self) -> Vector2f {
        self.size
    }

    #[script_bind]
    pub fn get_z_index(&self) -> u32 {
        self.z_index
    }

    #[script_bind]
    pub fn can_occlude_light(&self) -> bool {
        self.can_occlude_light.peek().value
    }

    #[script_bind]
    pub fn set_can_occlude_light(&mut self, can_occlude: bool) {
        self.can_occlude_light.set(can_occlude);
    }

    #[script_bind]
    pub fn get_transform(&self) -> Transform2d {
        self.transform.peek().value
    }

    #[script_bind]
    pub fn set_transform(&mut self, transform: Transform2d) {
        if transform == self.transform.peek().value {
            return;
        }
        self.transform.set(transform);
    }

    #[script_bind]
    pub fn get_position(&self) -> Vector2f {
        self.transform.peek().value.translation
    }

    #[script_bind]
    pub fn set_position(&mut self, position: Vector2f) {
        self.transform.update_in_place(|transform| { transform.translation = position });
    }

    #[script_bind]
    pub fn get_scale(&self) -> Vector2f {
        self.transform.peek().value.translation
    }

    #[script_bind]
    pub fn set_scale(&mut self, scale: Vector2f) {
        self.transform.update_in_place(|transform| { transform.scale = scale });
    }

    #[script_bind]
    pub fn get_rotation(&self) -> f32 {
        self.transform.peek().value.rotation
    }

    #[script_bind]
    pub fn set_rotation(&mut self, rotation_rads: f32) {
        self.transform.update_in_place(|transform| { transform.rotation = rotation_rads });
    }

    #[script_bind]
    pub fn get_velocity(&self) -> Vector2f {
        self.velocity
    }

    #[script_bind]
    pub fn set_velocity(&mut self, velocity: Vector2f) {
        self.velocity = velocity;
    }

    #[script_bind]
    pub fn add_velocity(&mut self, delta: Vector2f) {
        self.velocity += delta;
    }

    #[script_bind]
    pub fn get_sprite(&self) -> &Sprite {
        &self.sprite
    }

    #[script_bind]
    pub fn get_sprite_mut(&mut self) -> &mut Sprite {
        &mut self.sprite
    }
}

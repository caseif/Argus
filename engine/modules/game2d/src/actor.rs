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
use crate::collision::BoundingRect;
use crate::object::CommonObjectProperties;
use crate::sprite::Sprite;

#[script_bind(ref_only)]
pub struct Actor2d {
    pub(crate) common: CommonObjectProperties,
    pub(crate) can_occlude_light: Dirtiable<bool>,
    pub(crate) bounding_box: Dirtiable<BoundingRect>,
    pub(crate) velocity: Vector2f,
    pub(crate) transform: Dirtiable<Transform2d>,
}

#[script_bind]
impl Actor2d {
    pub(crate) fn new(
        sprite_defn_res: Resource,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        collision_layer: u64,
        collision_mask: u64,
    ) -> Actor2d {
        let sprite = Sprite::new(sprite_defn_res);

        Self {
            common: CommonObjectProperties {
                size,
                z_index,
                sprite,
                render_obj: None,
                collision_layer,
                collision_mask,
            },
            velocity: Vector2f::default(),
            can_occlude_light: Dirtiable::new(can_occlude_light),
            bounding_box: Dirtiable::new(BoundingRect {
                size,
                center: Vector2f::default(),
                rotation_rads: 0.0,
            }),
            transform: Default::default(),
        }
    }

    #[script_bind]
    pub fn get_size(&self) -> Vector2f {
        self.common.size
    }

    #[script_bind]
    pub fn get_z_index(&self) -> u32 {
        self.common.z_index
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
    pub fn get_bounding_box(&self) -> BoundingRect {
        self.bounding_box.peek().value
    }

    #[script_bind]
    pub fn set_bounding_box(&mut self, bounding_box: BoundingRect) {
        self.bounding_box.set(bounding_box);
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
        &self.common.sprite
    }

    #[script_bind]
    pub fn get_sprite_mut(&mut self) -> &mut Sprite {
        &mut self.common.sprite
    }
}

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
use argus_render::common::Transform2d;
use argus_resman::Resource;
use argus_util::math::Vector2f;
use argus_util::pool::Handle;
use crate::collision::BoundingRect;
use crate::object::CommonObjectProperties;
use crate::sprite::Sprite;

pub struct StaticObject2d {
    pub(crate) common: CommonObjectProperties,
    pub(crate) can_occlude_light: bool,
    pub(crate) bounding_box: BoundingRect,
    pub(crate) transform: Transform2d,
}

impl StaticObject2d {
    pub(crate) fn new(
        sprite_def_res: Resource,
        size: Vector2f,
        z_index: u32,
        can_occlude_light: bool,
        transform: Transform2d,
        collision_layer: u64,
        collision_mask: u64,
    ) -> Self {
        Self {
            common: CommonObjectProperties {
                sprite: Sprite::new(sprite_def_res),
                size,
                z_index,
                render_obj: None,
                collision_layer,
                collision_mask,
            },
            can_occlude_light,
            bounding_box: BoundingRect {
                size,
                center: Vector2f::default(),
                rotation_rads: 0.0,
            },
            transform,
        }
    }

    pub fn get_size(&self) -> Vector2f {
        self.common.size
    }

    pub fn get_z_index(&self) -> u32 {
        self.common.z_index
    }

    pub fn get_can_occlude_light(&self) -> bool {
        self.can_occlude_light
    }

    pub fn get_transform(&self) -> Transform2d {
        self.transform.clone()
    }

    pub fn get_sprite(&self) -> &Sprite {
        &self.common.sprite
    }

    pub fn get_sprite_mut(&mut self) -> &mut Sprite {
        &mut self.common.sprite
    }
    
    pub fn get_bounding_box(&self) -> &BoundingRect {
        &self.bounding_box
    }
}

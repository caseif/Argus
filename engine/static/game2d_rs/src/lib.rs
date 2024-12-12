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

use core_rustabi::argus::core::{register_update_callback, LifecycleStage, Ordering};
use num_enum::UnsafeFromPrimitive;
use resman_rustabi::argus::resman::ResourceManager;
use crate::constants::RESOURCE_TYPE_SPRITE;
use crate::sprite_loader::SpriteLoader;
use crate::world::World2d;

pub mod actor;
pub mod sprite;
pub mod static_object;
pub mod world;
pub mod world_layer;

mod constants;
mod sprite_loader;

#[no_mangle]
pub unsafe extern "C" fn update_lifecycle_game2d_rs(stage_ffi: core_rustabi::core_cabi::LifecycleStage) {
    let stage = unsafe { LifecycleStage::unchecked_transmute_from(stage_ffi) };

    match stage {
        LifecycleStage::Load => {}
        LifecycleStage::PreInit => {}
        LifecycleStage::Init => {
            ResourceManager::get_instance().register_loader(vec![RESOURCE_TYPE_SPRITE], SpriteLoader {});
            register_update_callback(World2d::render_worlds_c, Ordering::Standard);
        }
        LifecycleStage::PostInit => {}
        LifecycleStage::Running => {}
        LifecycleStage::PreDeinit => {}
        LifecycleStage::Deinit => {}
        LifecycleStage::PostDeinit => {}
    }
}

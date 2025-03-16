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

#![feature(used_with_arg)]

use argus_core::{register_module, register_update_callback, LifecycleStage, Ordering};
use argus_resman::ResourceManager;
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
mod light_point;

argus_logging::crate_logger!(LOGGER, "argus/game2d");

#[register_module(id = "game2d", depends(core, resman, scripting, wm, input, render))]
pub fn update_lifecycle_game2d(stage: LifecycleStage) {
    match stage {
        LifecycleStage::Load => {}
        LifecycleStage::PreInit => {}
        LifecycleStage::Init => {
            ResourceManager::instance().register_loader(vec![RESOURCE_TYPE_SPRITE], SpriteLoader {});
            register_update_callback(Box::new(World2d::simulate_worlds), Ordering::Standard);
            register_update_callback(Box::new(World2d::render_worlds), Ordering::Late);
        }
        LifecycleStage::PostInit => {}
        LifecycleStage::Running => {}
        LifecycleStage::PreDeinit => {}
        LifecycleStage::Deinit => {}
        LifecycleStage::PostDeinit => {}
    }
}

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

use std::collections::{BTreeMap, HashMap};
use argus_render::common::SceneType;
use argus_util::pool::Handle;
use crate::state::{ProcessedObject, RenderBucket, RenderBucketKey};
use crate::util::VulkanBuffer;

pub(crate) struct Scene2dState {
    pub(crate) scene_id: String,
    pub(crate) scene_type: SceneType,
    pub(crate) ubo: Option<VulkanBuffer>,
    pub(crate) render_buckets: BTreeMap<RenderBucketKey, RenderBucket>,
    pub(crate) processed_objs: HashMap<Handle, ProcessedObject>,
    pub(crate) visited: bool,
}

impl Scene2dState {
    pub(crate) fn new(scene_id: impl Into<String>) -> Self {
        Self {
            scene_id: scene_id.into(),
            scene_type: SceneType::TwoDim,
            ubo: None,
            render_buckets: BTreeMap::new(),
            processed_objs: Default::default(),
            visited: false,
        }
    }
}

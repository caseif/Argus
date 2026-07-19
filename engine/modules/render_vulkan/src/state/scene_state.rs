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
use vk_wrapper::vk;
use crate::state::{ProcessedObject, RenderBucket, RenderBucketKey};

pub(crate) struct Scene2dState<'ctx> {
    pub(crate) scene_id: String,
    pub(crate) scene_type: SceneType,
    pub(crate) ubo: Option<vk::Buffer<'ctx>>,
    pub(crate) render_buckets: BTreeMap<RenderBucketKey, RenderBucket<'ctx>>,
    pub(crate) processed_objs: HashMap<Handle, ProcessedObject<'ctx>>,
    pub(crate) visited: bool,
}

impl<'ctx> Scene2dState<'ctx> {
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

    pub fn destroy(self) {
        for (_, bucket) in self.render_buckets {
            bucket.destroy();
        }

        for (_, obj) in self.processed_objs {
            obj.destroy();
        }

        if let Some(buf) = self.ubo {
            buf.destroy();
        }
    }
}

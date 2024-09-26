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

use render_rustabi::argus::render::{ProcessedObjectMap, Scene2d};
use std::collections::BTreeMap;

use crate::argus::render_opengl_rust::state::{ProcessedObject, RenderBucket, RenderBucketKey};
use crate::argus::render_opengl_rust::util::buffer::GlBuffer;

pub(crate) struct Scene2dState {
    pub(crate) scene: Scene2d,
    pub(crate) ubo: Option<GlBuffer>,
    pub(crate) render_buckets: BTreeMap<RenderBucketKey, RenderBucket>,
    pub(crate) processed_objs: ProcessedObjectMap<ProcessedObject>,
}

impl<'a> Scene2dState {
    pub(crate) fn new(scene: &Scene2d) -> Self {
        Self {
            scene: scene.clone(),
            ubo: None,
            render_buckets: BTreeMap::new(),
            processed_objs: Default::default(),
        }
    }
}

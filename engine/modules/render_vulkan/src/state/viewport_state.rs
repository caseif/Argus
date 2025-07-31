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
use std::collections::HashMap;
use ash::vk;
use argus_render::common::{Matrix4x4, SceneType};
use argus_resman::ResourceIdentifier;
use crate::util::{CommandBufferInfo, FramebufferGrouping, VulkanBuffer};
use crate::util::defines::MAX_FRAMES_IN_FLIGHT;

#[derive(Default)]
pub(crate) struct PerFrameData {
    pub(crate) view_matrix_dirty: bool,

    pub(crate) command_buf: Option<CommandBufferInfo>,

    pub(crate) composite_fence: vk::Fence,

    pub(crate) front_fb: Option<FramebufferGrouping>,
    pub(crate) back_fb: Option<FramebufferGrouping>,

    pub(crate) scene_ubo: Option<VulkanBuffer>,
    pub(crate) scene_ubo_dirty: bool,

    pub(crate) viewport_ubo: Option<VulkanBuffer>,

    pub(crate) rebuild_semaphore: vk::Semaphore,
    pub(crate) draw_semaphore: vk::Semaphore,

    pub(crate) material_desc_sets: HashMap<ResourceIdentifier, Vec<vk::DescriptorSet>>,
    pub(crate) composite_desc_sets: Vec<vk::DescriptorSet>,
}

pub(crate) struct ViewportState {
    pub(crate) viewport_id: u32,
    pub(crate) ty: SceneType,
    pub(crate) visited: bool,
    pub(crate) per_frame: [PerFrameData; MAX_FRAMES_IN_FLIGHT],
}

impl ViewportState {
    pub(crate) fn new(viewport_id: u32) -> Self {
        Self {
            viewport_id,
            ty: SceneType::TwoDim,
            visited: false,
            per_frame: Default::default(),
        }
    }
}

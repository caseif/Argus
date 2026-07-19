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
use argus_render::common::SceneType;
use argus_resman::ResourceIdentifier;
use vk_wrapper::*;

#[derive(Default)]
pub(crate) struct PerFrameData<'ctx> {
    pub(crate) view_matrix_dirty: bool,

    pub(crate) command_buf: Option<vk::CommandBuffer<'ctx>>,

    pub(crate) composite_fence: Option<vk::Fence<'ctx>>,

    pub(crate) front_fb: Option<vk::Framebuffer<'ctx>>,
    pub(crate) back_fb: Option<vk::Framebuffer<'ctx>>,

    pub(crate) scene_ubo: Option<vk::Buffer<'ctx>>,
    pub(crate) scene_ubo_dirty: bool,

    pub(crate) viewport_ubo: Option<vk::Buffer<'ctx>>,

    pub(crate) rebuild_semaphore: Option<vk::Semaphore<'ctx>>,
    pub(crate) draw_semaphore: Option<vk::Semaphore<'ctx>>,

    pub(crate) material_desc_sets: HashMap<ResourceIdentifier, vk::DescriptorSetGroup<'ctx>>,
    pub(crate) composite_desc_sets: Option<vk::DescriptorSetGroup<'ctx>>,
}

pub(crate) struct ViewportState<'ctx> {
    #[allow(dead_code)]
    pub(crate) viewport_id: u32,
    #[allow(dead_code)]
    pub(crate) ty: SceneType,
    pub(crate) visited: bool,
    pub(crate) per_frame: [PerFrameData<'ctx>; vk::MAX_FRAMES_IN_FLIGHT],
}

impl<'ctx> ViewportState<'ctx> {
    pub(crate) fn new(
        viewport_id: u32
    ) -> Self {
        Self {
            viewport_id,
            ty: SceneType::TwoDim,
            visited: false,
            per_frame: Default::default(),
        }
    }

    pub fn destroy(self, desc_pool: &vk::DescriptorPool, cmd_pool: &vk::CommandPool) {
        for mut frame_state in self.per_frame {
            frame_state.composite_fence.unwrap().destroy();
            if let Some(fb) = frame_state.front_fb.take() {
                fb.destroy();
            }
            if let Some(fb) = frame_state.back_fb.take() {
                fb.destroy();
            }

            if let Some(buf) = frame_state.viewport_ubo {
                buf.destroy();
            }
            if let Some(buf) = frame_state.scene_ubo {
                buf.destroy();
            }

            if let Some(composite_ds_group) = frame_state.composite_desc_sets {
                composite_ds_group.destroy(desc_pool).unwrap();
            }
            for (_, ds) in frame_state.material_desc_sets {
                ds.destroy(desc_pool).unwrap();
            }

            if let Some(cmd_buf) = frame_state.command_buf.take() {
                cmd_buf.destroy(cmd_pool);
            }
        }
    }
}

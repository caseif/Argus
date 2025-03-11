use argus_resman::Resource;
use argus_util::math::{Vector2f, Vector2u};
use crate::setup::device::VulkanDevice;
use crate::util::VulkanBuffer;

pub(crate) struct ProcessedObject {
    pub(crate) material_res: Resource,
    pub(crate) atlas_stride: Vector2f,
    pub(crate) z_index: u32,
    pub(crate) light_opacity: f32,
    pub(crate) vertex_count: u32,

    pub(crate) anim_frame: Vector2u,

    pub(crate) staging_buffer: Option<VulkanBuffer>,
    pub(crate) newly_created: bool,
    pub(crate) visited: bool,
    pub(crate) updated: bool,
    pub(crate) anim_frame_updated: bool,
}

impl ProcessedObject {
    pub(crate) fn new(
        material_res: Resource,
        atlas_stride: Vector2f,
        z_index: u32,
        light_opacity: f32,
        vertex_count: u32,
    ) -> Self {
        Self {
            material_res,
            atlas_stride,
            z_index,
            light_opacity,
            vertex_count,
            anim_frame: Default::default(),
            staging_buffer: None,
            newly_created: true,
            visited: false,
            updated: false,
            anim_frame_updated: false,
        }
    }
    
    pub(crate) fn destroy(self, device: &VulkanDevice) {
        if let Some(buf) = self.staging_buffer {
            buf.destroy(device);
        }
    }
}
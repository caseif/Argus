use std::cmp::Ordering;
use argus_resman::{Resource, ResourceIdentifier};
use argus_util::math::{Vector2f, Vector2i};
use argus_util::pool::Handle;
use crate::setup::device::VulkanDevice;
use crate::util::VulkanBuffer;

#[derive(Clone, Eq, Hash, PartialEq)]
pub(crate) struct RenderBucketKey {
    pub(crate) material_uid: ResourceIdentifier,
    pub(crate) atlas_stride: Vector2i,
    pub(crate) z_index: u32,
    pub(crate) light_opacity: i32,
}

impl RenderBucketKey {
    pub(crate) fn new(
        material_uid: ResourceIdentifier,
        atlas_stride: &Vector2f,
        z_index: u32,
        light_opacity: f32
    ) -> Self {
        Self {
            material_uid,
            atlas_stride: Vector2i {
                x: (atlas_stride.x * 1_000_000.0) as i32,
                y: (atlas_stride.y * 1_000_000.0) as i32,
            },
            z_index,
            light_opacity: (light_opacity * 1_000_000.0) as i32,
        }
    }
}

pub(crate) struct RenderBucket {
    pub(crate) material_res: Resource,
    pub(crate) atlas_stride: Vector2f,
    pub(crate) z_index: u32,
    pub(crate) light_opacity: f32,

    pub(crate) objects: Vec<Handle>,
    pub(crate) vertex_buffer: Option<VulkanBuffer>,
    pub(crate) staging_vertex_buffer: Option<VulkanBuffer>,
    pub(crate) anim_frame_buffer: Option<VulkanBuffer>,
    pub(crate) staging_anim_frame_buffer: Option<VulkanBuffer>,
    pub(crate) vertex_count: u32,

    pub(crate) ubo_buffer: Option<VulkanBuffer>,

    pub(crate) needs_rebuild: bool,
}

impl RenderBucket {
    pub(crate) fn new(
        material_res: Resource,
        atlas_stride: Vector2f,
        z_index: u32,
        light_opacity: f32,
    ) -> Self {
        Self {
            material_res,
            atlas_stride,
            z_index,
            light_opacity,
            objects: Vec::new(),
            vertex_buffer: None,
            staging_vertex_buffer: None,
            anim_frame_buffer: None,
            staging_anim_frame_buffer: None,
            vertex_count: 0,
            ubo_buffer: None,
            needs_rebuild: false,
        }
    }
    
    pub(crate) fn destroy(self, device: &VulkanDevice) {        
        if let Some(buf) = self.vertex_buffer {
            buf.destroy(device);
        }
        if let Some(buf) = self.staging_vertex_buffer {
            buf.destroy(device);
        }
        if let Some(buf) = self.anim_frame_buffer {
            buf.destroy(device);
        }
        if let Some(buf) = self.staging_anim_frame_buffer {
            buf.destroy(device);
        }
        if let Some(buf) = self.ubo_buffer {
            buf.destroy(device);
        }
    }
}

impl PartialOrd<Self> for RenderBucketKey {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for RenderBucketKey {
    fn cmp(&self, other: &Self) -> Ordering {
        self.z_index.cmp(&other.z_index)
            .then(self.light_opacity.cmp(&other.light_opacity))
            .then(self.atlas_stride.cmp(&other.atlas_stride))
            .then(self.material_uid.cmp(&other.material_uid))
    }
}

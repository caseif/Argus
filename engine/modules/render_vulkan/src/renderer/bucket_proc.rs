use std::collections::HashMap;
use ash::vk;
use argus_render::constants::*;
use argus_resman::ResourceIdentifier;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::state::Scene2dState;
use crate::util::{CommandBufferInfo, PipelineInfo, VulkanBuffer, MEM_CLASS_DEVICE_RO, MEM_CLASS_DEVICE_RW};
use crate::util::defines::*;

pub(crate) fn fill_buckets(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    scene_state: &mut Scene2dState,
    copy_cmd_buf: &CommandBufferInfo,
    material_pipelines: &HashMap<ResourceIdentifier, PipelineInfo>,
) {
    scene_state.render_buckets.retain(|_, bucket| !bucket.objects.is_empty());

    for (_, bucket) in &mut scene_state.render_buckets {
        if bucket.ubo_buffer.is_none() {
            bucket.ubo_buffer = Some(VulkanBuffer::new(
                instance,
                device,
                SHADER_UBO_OBJ_LEN as vk::DeviceSize,
                vk::BufferUsageFlags::UNIFORM_BUFFER,
                MEM_CLASS_DEVICE_RW,
            ).unwrap());
            let ubo_buffer = bucket.ubo_buffer.as_mut().unwrap();

            // we assume these values will never change

            let uv_stride = [bucket.atlas_stride.x, bucket.atlas_stride.y];
            ubo_buffer.write(
                device,
                &uv_stride,
                SHADER_UNIFORM_OBJ_UV_STRIDE_OFF as vk::DeviceSize,
            ).unwrap();

            ubo_buffer.write(
                device,
                &[bucket.light_opacity],
                SHADER_UNIFORM_OBJ_LIGHT_OPACITY_OFF as vk::DeviceSize,
            ).unwrap();
        }

        let pipeline = material_pipelines.get(&bucket.material_res.get_prototype().uid)
            .expect("Cannot find material pipeline");

        let attr_position_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_POSITION);
        let attr_normal_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_NORMAL);
        let attr_color_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_COLOR);
        let attr_texcoord_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_TEXCOORD);
        //let attr_anim_frame_loc = pipeline.reflection.get_attr_loc(SHADER_ATTRIB_ANIM_FRAME);

        let vertex_comps = attr_position_loc.map(|_| SHADER_ATTRIB_POSITION_LEN).unwrap_or(0)
            + attr_normal_loc.map(|_| SHADER_ATTRIB_NORMAL_LEN).unwrap_or(0)
            + attr_color_loc.map(|_| SHADER_ATTRIB_COLOR_LEN).unwrap_or(0)
            + attr_texcoord_loc.map(|_| SHADER_ATTRIB_TEXCOORD_LEN).unwrap_or(0);

        let mut anim_frame_buf_len: vk::DeviceSize = 0;
        if bucket.needs_rebuild {
            let mut vertex_buf_len: vk::DeviceSize = 0;
            for obj_handle in &bucket.objects {
                let obj = &scene_state.processed_objs[obj_handle];
                
                vertex_buf_len += obj.staging_buffer.as_ref().unwrap().len();
                anim_frame_buf_len += (
                    obj.vertex_count *
                        SHADER_ATTRIB_ANIM_FRAME_LEN *
                        size_of::<f32>() as u32
                ) as vk::DeviceSize;
            }

            assert!(vertex_buf_len <= i32::MAX as vk::DeviceSize, "Buffer length is too big");

            if let Some(buf) = bucket.vertex_buffer.take() {
                buf.destroy(device);
            }
            bucket.vertex_buffer.replace(VulkanBuffer::new(
                instance,
                device,
                vertex_buf_len,
                {
                    vk::BufferUsageFlags::TRANSFER_DST |
                    vk::BufferUsageFlags::VERTEX_BUFFER
                },
                MEM_CLASS_DEVICE_RO,
            ).unwrap());
            if let Some(buf) = bucket.staging_vertex_buffer.take() {
                buf.destroy(device);
            }
            bucket.staging_vertex_buffer.replace(VulkanBuffer::new(
                instance,
                device,
                vertex_buf_len,
                {
                    vk::BufferUsageFlags::TRANSFER_SRC |
                    vk::BufferUsageFlags::TRANSFER_DST |
                    vk::BufferUsageFlags::VERTEX_BUFFER
                },
                MEM_CLASS_DEVICE_RO,
            ).unwrap());

            let stride = vertex_comps * size_of::<f32>() as u32;
            assert!(stride <= i32::MAX as u32, "Vertex stride is too big");

            assert!(
                anim_frame_buf_len <= i32::MAX as vk::DeviceSize,
                "Animation frame buffer length is too big",
            );
            if let Some(buf) = bucket.anim_frame_buffer.take() {
                buf.destroy(device);
            }
            bucket.anim_frame_buffer.replace(VulkanBuffer::new(
                instance,
                device,
                anim_frame_buf_len,
                {
                    vk::BufferUsageFlags::TRANSFER_DST |
                    vk::BufferUsageFlags::VERTEX_BUFFER
                },
                MEM_CLASS_DEVICE_RO,
            ).unwrap());
            if let Some(buf) = bucket.staging_anim_frame_buffer.take() {
                buf.destroy(device);
            }
            bucket.staging_anim_frame_buffer.replace(VulkanBuffer::new(
                instance,
                device,
                anim_frame_buf_len,
                {
                    vk::BufferUsageFlags::TRANSFER_SRC |
                    vk::BufferUsageFlags::VERTEX_BUFFER
                },
                MEM_CLASS_DEVICE_RW,
            ).unwrap());
        } else {
            anim_frame_buf_len = (
                bucket.vertex_count *
                    SHADER_ATTRIB_ANIM_FRAME_LEN *
                    size_of::<f32>() as u32
            ) as vk::DeviceSize;
        }

        bucket.vertex_count = 0;

        let mut anim_buf_updated = false;

        let mut offset: vk::DeviceSize = 0;
        let mut anim_frame_off: usize = 0;

        let mut anim_frame_buf = bucket.staging_anim_frame_buffer.as_mut().unwrap().map(
            device,
            0,
            vk::WHOLE_SIZE,
            vk::MemoryMapFlags::empty(),
        )
            .unwrap();
        let anim_frame_buf_map = anim_frame_buf.as_slice_mut::<f32>();

        for obj_handle in &mut bucket.objects {
            let obj = scene_state.processed_objs.get_mut(obj_handle).unwrap();
            
            let staging_buffer = obj.staging_buffer.as_mut().unwrap();

            if bucket.needs_rebuild || obj.updated {
                assert!(
                    staging_buffer.len() <= usize::MAX as vk::DeviceSize,
                    "Buffer offset is too big",
                );

                bucket.staging_vertex_buffer.as_mut().unwrap().copy_from(
                    device,
                    &staging_buffer,
                    0, // src offset
                    copy_cmd_buf,
                    offset, // dst offset
                    vk::WHOLE_SIZE,
                );

                obj.updated = false;
            }

            if bucket.needs_rebuild || obj.anim_frame_updated {                
                for _ in 0..obj.vertex_count {
                    anim_frame_buf_map[anim_frame_off] = obj.anim_frame.x as f32;
                    anim_frame_buf_map[anim_frame_off + 1] = obj.anim_frame.y as f32;
                    anim_frame_off += 2;
                }
                obj.anim_frame_updated = false;
                anim_buf_updated = true;
            } else {
                anim_frame_off += (obj.vertex_count * SHADER_ATTRIB_ANIM_FRAME_LEN) as usize;
            }

            offset += staging_buffer.len();

            bucket.vertex_count += obj.vertex_count;
        }
        drop(anim_frame_buf);

        bucket.vertex_buffer.as_mut().unwrap().copy_from(
            device,
            bucket.staging_vertex_buffer.as_ref().unwrap(),
            0, // src offset
            copy_cmd_buf,
            0, // dst offset
            vk::WHOLE_SIZE,
        );
        if anim_buf_updated {
            assert!(
                anim_frame_buf_len <= i32::MAX as vk::DeviceSize,
                "Animated frame buffer length is too big",
            );
            bucket.anim_frame_buffer.as_mut().unwrap().copy_from(
                device,
                bucket.staging_anim_frame_buffer.as_ref().unwrap(),
                0, // src offset
                copy_cmd_buf,
                0, // dst offset
                anim_frame_buf_len,
            );
        }

        bucket.needs_rebuild = false;
    }
}
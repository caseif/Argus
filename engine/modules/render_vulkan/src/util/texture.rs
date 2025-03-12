use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::util::{CommandBufferInfo, VulkanBuffer, VulkanImage, MEM_CLASS_DEVICE_RW};
use argus_render::common::TextureData;
use argus_resman::Resource;
use argus_util::math::Vector2u;
use ash::vk;

pub(crate) struct PreparedTexture {
    uid: String,
    pub(crate) image: VulkanImage,
    pub(crate) sampler: vk::Sampler,
    staging_buf: VulkanBuffer,
}

pub(crate) fn prepare_texture(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    cmd_buf: &CommandBufferInfo,
    texture_res: Resource,
) -> Result<PreparedTexture, String> {
    let texture = texture_res.get::<TextureData>().unwrap();

    let channels = 4;
    let image_size = (texture.get_width() * texture.get_height() * channels) as vk::DeviceSize;

    let format = vk::Format::R8G8B8A8_SRGB;

    let image = VulkanImage::create_image_with_view(
        instance,
        device,
        format,
        Vector2u::new(texture.get_width(), texture.get_height()),
        vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::SAMPLED,
        vk::ImageAspectFlags::COLOR
    )?;

    let mut staging_buf = VulkanBuffer::new(
        instance,
        device,
        image_size,
        vk::BufferUsageFlags::TRANSFER_SRC,
        MEM_CLASS_DEVICE_RW,
    )?;

    {
        let bytes_per_pixel = 4;
        let bytes_per_row = texture.get_width() * bytes_per_pixel;
        let mut mapped = staging_buf.map(device, 0, vk::WHOLE_SIZE, vk::MemoryMapFlags::empty())
            .map_err(|err| err.to_string())?;
        for y in 0..texture.get_height() {
            let dst = mapped.offset_mut(y * bytes_per_row);
            dst.copy_from_slice(&texture.get_pixel_data()[(y * bytes_per_row) as usize..]);
        }
    }

    let image_sr_layers = vk::ImageSubresourceLayers::default()
        .aspect_mask(vk::ImageAspectFlags::COLOR)
        .mip_level(0)
        .base_array_layer(0)
        .layer_count(1);
    let region = vk::BufferImageCopy::default()
        .buffer_offset(0)
        .buffer_row_length(0)
        .buffer_image_height(0)
        .image_subresource(image_sr_layers)
        .image_offset(vk::Offset3D { x: 0, y: 0, z: 0 })
        .image_extent(vk::Extent3D {
            width: texture.get_width(),
            height: texture.get_height(),
            depth: 1,
        });

    image.perform_transition(
        device,
        cmd_buf,
        vk::ImageLayout::UNDEFINED,
        vk::ImageLayout::TRANSFER_DST_OPTIMAL,
        vk::AccessFlags::empty(),
        vk::AccessFlags::TRANSFER_WRITE,
        vk::PipelineStageFlags::TOP_OF_PIPE,
        vk::PipelineStageFlags::TRANSFER,
    );

    unsafe {
        device.logical_device.cmd_copy_buffer_to_image(
            cmd_buf.get_handle(),
            staging_buf.get_handle(),
            image.get_vk_image(),
            vk::ImageLayout::TRANSFER_DST_OPTIMAL,
            &[region],
        );
    }

    image.perform_transition(
        device,
        cmd_buf,
        vk::ImageLayout::TRANSFER_DST_OPTIMAL,
        vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        vk::AccessFlags::TRANSFER_WRITE,
        vk::AccessFlags::SHADER_READ,
        vk::PipelineStageFlags::TRANSFER,
        vk::PipelineStageFlags::FRAGMENT_SHADER,
    );

    let sampler_info = vk::SamplerCreateInfo::default()
        .mag_filter(vk::Filter::NEAREST)
        .min_filter(vk::Filter::NEAREST)
        .address_mode_u(vk::SamplerAddressMode::CLAMP_TO_EDGE)
        .address_mode_v(vk::SamplerAddressMode::CLAMP_TO_EDGE)
        .address_mode_w(vk::SamplerAddressMode::CLAMP_TO_EDGE)
        .anisotropy_enable(false)
        .max_anisotropy(0.0)
        .border_color(vk::BorderColor::INT_OPAQUE_BLACK)
        .unnormalized_coordinates(false)
        .compare_enable(false)
        .compare_op(vk::CompareOp::ALWAYS)
        .mipmap_mode(vk::SamplerMipmapMode::LINEAR)
        .mip_lod_bias(0.0)
        .min_lod(0.0)
        .max_lod(0.0);

    let sampler = unsafe {
        device.logical_device.create_sampler(&sampler_info, None)
            .map_err(|err| err.to_string())?
    };

    Ok(PreparedTexture {
        uid: texture_res.get_prototype().uid.to_string(),
        image,
        sampler,
        staging_buf,
    })
}

pub(crate) fn destroy_texture(device: &VulkanDevice, texture: PreparedTexture) {
    unsafe { device.logical_device.destroy_sampler(texture.sampler, None) };
    texture.image.destroy(device);
    texture.staging_buf.destroy(device);
}

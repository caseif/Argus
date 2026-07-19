use argus_util::math::Vector2u;
use crate::vk;
use crate::vk::Wrapper;

pub struct PreparedTexture<'ctx> {
    pub image: vk::Image<'ctx>,
    pub sampler: vk::Sampler<'ctx>,
    staging_buf: vk::Buffer<'ctx>,
}

impl<'ctx> PreparedTexture<'ctx> {
    pub fn destroy(self) {
        self.sampler.destroy();
        self.image.destroy();
        self.staging_buf.destroy();
    }
}

pub fn prepare_texture<'ctx>(
    device: &'ctx vk::Device<'ctx>,
    cmd_buf: &vk::CommandBuffer<'ctx>,
    width: u32,
    height: u32,
    pixel_data: &[u8],
) -> Result<PreparedTexture<'ctx>, String> {
    let channels = 4;
    let image_size = (width * height * channels) as vk::DeviceSize;

    let format = vk::Format::R8G8B8A8_SRGB;

    let image = vk::Image::create_with_view(
        device,
        format,
        Vector2u::new(width, height),
        vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::SAMPLED,
        vk::ImageAspectFlags::COLOR
    )?;

    let mut staging_buf = vk::Buffer::new(
        device,
        image_size,
        vk::BufferUsageFlags::TRANSFER_SRC,
        vk::MEM_CLASS_DEVICE_RW,
    )?;

    {
        let bytes_per_pixel = 4;
        let bytes_per_row = width * bytes_per_pixel;
        let mut mapped = staging_buf.map(device, 0, vk::WHOLE_SIZE, vk::MemoryMapFlags::empty())
            .map_err(|err| err.to_string())?;
        for y in 0..height {
            let dst = mapped.offset_mut(y * bytes_per_row);
            dst.copy_from_slice(&pixel_data[(y * bytes_per_row) as usize..]);
        }
    }

    let image_sr_layers = ash::vk::ImageSubresourceLayers::default()
        .aspect_mask(vk::ImageAspectFlags::COLOR)
        .mip_level(0)
        .base_array_layer(0)
        .layer_count(1);
    let region = ash::vk::BufferImageCopy::default()
        .buffer_offset(0)
        .buffer_row_length(0)
        .buffer_image_height(0)
        .image_subresource(image_sr_layers)
        .image_offset(ash::vk::Offset3D { x: 0, y: 0, z: 0 })
        .image_extent(ash::vk::Extent3D {
            width,
            height,
            depth: 1,
        });

    image.perform_transition(
        cmd_buf,
        ash::vk::ImageLayout::UNDEFINED,
        ash::vk::ImageLayout::TRANSFER_DST_OPTIMAL,
        ash::vk::AccessFlags::empty(),
        ash::vk::AccessFlags::TRANSFER_WRITE,
        ash::vk::PipelineStageFlags::TOP_OF_PIPE,
        ash::vk::PipelineStageFlags::TRANSFER,
    );

    unsafe {
        device.get_underlying().cmd_copy_buffer_to_image(
            cmd_buf.get_underlying(),
            staging_buf.get_underlying(),
            image.get_underlying(),
            vk::ImageLayout::TRANSFER_DST_OPTIMAL,
            &[region],
        );
    }

    image.perform_transition(
        cmd_buf,
        ash::vk::ImageLayout::TRANSFER_DST_OPTIMAL,
        ash::vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        ash::vk::AccessFlags::TRANSFER_WRITE,
        ash::vk::AccessFlags::SHADER_READ,
        ash::vk::PipelineStageFlags::TRANSFER,
        ash::vk::PipelineStageFlags::FRAGMENT_SHADER,
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

    let sampler = vk::Sampler::create(device, &sampler_info)?;

    Ok(PreparedTexture {
        image,
        sampler,
        staging_buf,
    })
}

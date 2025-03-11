use ash::vk;
use argus_util::math::Vector2u;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::util::{find_memory_type, CommandBufferInfo, MEM_CLASS_DEVICE_RO};

pub(crate) struct VulkanImage {
    size: Vector2u,
    format: vk::Format,
    vk_image: vk::Image,
    vk_view: vk::ImageView,
    is_destroyed: bool,
}

impl VulkanImage {
    pub(crate) fn create_image_with_view(
        instance: &VulkanInstance,
        device: &VulkanDevice,
        format: vk::Format,
        size: Vector2u,
        usage: vk::ImageUsageFlags,
        aspect_mask: vk::ImageAspectFlags
    ) -> Result<VulkanImage, String> {
        unsafe {
            let vk_image = create_vk_image(instance, device, format, &size, usage)?;
            let vk_view = create_vk_image_view(device, vk_image, format, aspect_mask)?;
            let image_info = VulkanImage {
                size,
                format,
                vk_image,
                vk_view,
                is_destroyed: false,
            };
            Ok(image_info)
        }
    }
    
    pub(crate) fn get_size(&self) -> &Vector2u {
        &self.size
    }

    pub(crate) unsafe fn get_vk_image(&self) -> vk::Image {
        self.vk_image
    }

    pub(crate) unsafe fn get_vk_image_view(&self) -> vk::ImageView {
        self.vk_view
    }

    pub(crate) fn destroy(mut self, device: &VulkanDevice) {
        unsafe {
            destroy_vk_image_view(device, self.vk_view);
            destroy_vk_image(device, self.vk_image);
        }
        self.is_destroyed = true;
    }

    pub(crate) fn perform_transition(
        &self,
        device: &VulkanDevice,
        cmd_buf: &CommandBufferInfo,
        old_layout: vk::ImageLayout,
        new_layout: vk::ImageLayout,
        src_access: vk::AccessFlags,
        dst_access: vk::AccessFlags,
        src_stage: vk::PipelineStageFlags,
        dst_stage: vk::PipelineStageFlags,
    ) {
        unsafe {
            perform_vk_image_transition(
                device,
                self.vk_image,
                cmd_buf,
                old_layout,
                new_layout,
                src_access,
                dst_access,
                src_stage,
                dst_stage
            );
        }
    }
}

impl Drop for VulkanImage {
    fn drop(&mut self) {
        if cfg!(debug_assertions) && !self.is_destroyed {
            panic!("Vulkan image was dropped without being destroyed!")
        }
    }
}

pub(crate) unsafe fn create_vk_image(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    format: vk::Format,
    size: &Vector2u,
    usage: vk::ImageUsageFlags
) -> Result<vk::Image, String> {
    let extent = vk::Extent3D { width: size.x, height: size.y, depth: 1 };
    let qf_indices = vec![device.queue_indices.graphics_family];

    let image_info = vk::ImageCreateInfo::default()
        .image_type(vk::ImageType::TYPE_2D)
        .format(format)
        .extent(extent)
        .mip_levels(1)
        .array_layers(1)
        .samples(vk::SampleCountFlags::TYPE_1)
        .tiling(vk::ImageTiling::OPTIMAL)
        .usage(usage)
        .sharing_mode(vk::SharingMode::EXCLUSIVE)
        .queue_family_indices(&qf_indices)
        .initial_layout(vk::ImageLayout::UNDEFINED);

    let image = {
        let _queue_lock = device.queue_mutexes.graphics_family.lock().unwrap();
        device.logical_device.create_image(&image_info, None)
            .map_err(|err| err.to_string())?
    };

    let mem_reqs = device.logical_device.get_image_memory_requirements(image);

    let Some(mem_type) = find_memory_type(
        instance,
        device,
        mem_reqs.memory_type_bits,
        mem_reqs.size,
        MEM_CLASS_DEVICE_RO
    )
    else {
        return Err("Failed to find suitable memory type for vkImage".to_owned());
    };
    let alloc_info = vk::MemoryAllocateInfo::default()
        .allocation_size(mem_reqs.size)
        .memory_type_index(mem_type);

    let image_memory = device.logical_device.allocate_memory(&alloc_info, None)
        .map_err(|err| err.to_string())?;

    device.logical_device.bind_image_memory(image, image_memory, 0)
        .map_err(|err| err.to_string())?;

    Ok(image)
}

pub(crate) unsafe fn create_vk_image_view(
    device: &VulkanDevice,
    image: vk::Image,
    format: vk::Format,
    aspect_mask: vk::ImageAspectFlags
) -> Result<vk::ImageView, String> {
    let components = vk::ComponentMapping::default()
        .r(vk::ComponentSwizzle::IDENTITY)
        .g(vk::ComponentSwizzle::IDENTITY)
        .b(vk::ComponentSwizzle::IDENTITY)
        .a(vk::ComponentSwizzle::IDENTITY);
    let sr_range = vk::ImageSubresourceRange::default()
        .aspect_mask(aspect_mask)
        .base_mip_level(0)
        .level_count(1)
        .base_array_layer(0)
        .layer_count(1);

    let view_info = vk::ImageViewCreateInfo::default()
        .image(image)
        .view_type(vk::ImageViewType::TYPE_2D)
        .format(format)
        .components(components)
        .subresource_range(sr_range);

    let view = device.logical_device.create_image_view(&view_info, None)
        .map_err(|err| err.to_string())?;
    Ok(view)
}

pub(crate) unsafe fn destroy_vk_image(device: &VulkanDevice, image: vk::Image) {
    device.logical_device.destroy_image(image, None);
}

pub(crate) unsafe fn destroy_vk_image_view(device: &VulkanDevice, view: vk::ImageView) {
    unsafe { device.logical_device.destroy_image_view(view, None); }
}

unsafe fn perform_vk_image_transition(
    device: &VulkanDevice,
    image: vk::Image,
    cmd_buf: &CommandBufferInfo,
    old_layout: vk::ImageLayout,
    new_layout: vk::ImageLayout,
    src_access: vk::AccessFlags,
    dst_access: vk::AccessFlags,
    src_stage: vk::PipelineStageFlags,
    dst_stage: vk::PipelineStageFlags,
) {
    let sr_range = vk::ImageSubresourceRange::default()
        .aspect_mask(vk::ImageAspectFlags::COLOR)
        .base_mip_level(0)
        .level_count(1)
        .base_array_layer(0)
        .layer_count(1);

    let barrier = vk::ImageMemoryBarrier::default()
        .old_layout(old_layout)
        .new_layout(new_layout)
        .src_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
        .dst_queue_family_index(vk::QUEUE_FAMILY_IGNORED)
        .src_access_mask(src_access)
        .dst_access_mask(dst_access)
        .image(image)
        .subresource_range(sr_range);

    device.logical_device.cmd_pipeline_barrier(
        cmd_buf.get_handle(),
        src_stage,
        dst_stage,
        vk::DependencyFlags::empty(),
        &[],
        &[],
        &[barrier],
    );
}

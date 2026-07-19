use argus_util::math::Vector2u;
use crate::vk;

pub use ash::vk::ImageAspectFlags;
pub use ash::vk::ImageLayout;
pub use ash::vk::ImageUsageFlags;
use crate::vk::Wrapper;

pub struct Image<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::Image,
    size: Vector2u,
    #[allow(dead_code)]
    format: ash::vk::Format,
    vk_mem: ash::vk::DeviceMemory,
    view: Option<ImageView<'ctx>>,
    is_destroyed: bool,
}

impl<'ctx> Wrapper for Image<'ctx> {
    type Underlying = ash::vk::Image;

    unsafe fn get_underlying(&self) -> ash::vk::Image {
        self.underlying
    }
}

impl<'ctx> Image<'ctx> {
    pub fn create(
        device: &'ctx vk::Device<'ctx>,
        format: vk::Format,
        size: &Vector2u,
        usage: vk::ImageUsageFlags
    ) -> Result<Self, String> {
        let extent = ash::vk::Extent3D { width: size.x, height: size.y, depth: 1 };
        let qf_indices = vec![device.queue_indices.graphics_family];

        let image_info = ash::vk::ImageCreateInfo::default()
            .image_type(ash::vk::ImageType::TYPE_2D)
            .format(format)
            .extent(extent)
            .mip_levels(1)
            .array_layers(1)
            .samples(ash::vk::SampleCountFlags::TYPE_1)
            .tiling(ash::vk::ImageTiling::OPTIMAL)
            .usage(usage)
            .sharing_mode(ash::vk::SharingMode::EXCLUSIVE)
            .queue_family_indices(&qf_indices)
            .initial_layout(ash::vk::ImageLayout::UNDEFINED);

        let image = {
            let _queue_lock = device.queue_mutexes.graphics_family.lock().unwrap();
            device.create_vk_image(&image_info)
                .map_err(|err| err.to_string())?
        };

        let mem_reqs = unsafe { device.get_underlying().get_image_memory_requirements(image) };

        let Some(mem_type) = vk::find_memory_type(
            device,
            mem_reqs.memory_type_bits,
            mem_reqs.size,
            vk::MEM_CLASS_DEVICE_RO
        )
        else {
            return Err("Failed to find suitable memory type for vkImage".to_owned());
        };
        let alloc_info = ash::vk::MemoryAllocateInfo::default()
            .allocation_size(mem_reqs.size)
            .memory_type_index(mem_type);

        let image_memory = unsafe { device.get_underlying().allocate_memory(&alloc_info, None) }
            .map_err(|err| err.to_string())?;

        unsafe { device.get_underlying().bind_image_memory(image, image_memory, 0) }
            .map_err(|err| err.to_string())?;

        Ok(Self {
            device,
            underlying: image,
            size: *size,
            format,
            vk_mem: image_memory,
            view: None,
            is_destroyed: false,
        })
    }

    pub fn create_with_view(
        device: &'ctx vk::Device<'ctx>,
        format: vk::Format,
        size: Vector2u,
        usage: vk::ImageUsageFlags,
        aspect_mask: vk::ImageAspectFlags
    ) -> Result<Image<'ctx>, String> {
        let mut image = Self::create(device, format, &size, usage)?;
        image.view = Some(ImageView::create(device, &image, format, aspect_mask)?);
        Ok(image)
    }

    pub fn destroy(mut self) {
        unsafe {
            if let Some(view) = self.view.take() {
                view.destroy();
            }
            self.device.get_underlying().destroy_image(self.underlying, None);
            self.device.get_underlying().free_memory(self.vk_mem, None);
        }
        self.is_destroyed = true;
    }

    pub fn get_size(&self) -> &Vector2u {
        &self.size
    }

    pub fn get_view(&self) -> Option<&ImageView<'_>> {
        self.view.as_ref()
    }

    pub fn perform_transition(
        &self,
        cmd_buf: &vk::CommandBuffer,
        old_layout: vk::ImageLayout,
        new_layout: vk::ImageLayout,
        src_access: vk::AccessFlags,
        dst_access: vk::AccessFlags,
        src_stage: vk::PipelineStageFlags,
        dst_stage: vk::PipelineStageFlags,
    ) {
        let sr_range = ash::vk::ImageSubresourceRange::default()
            .aspect_mask(vk::ImageAspectFlags::COLOR)
            .base_mip_level(0)
            .level_count(1)
            .base_array_layer(0)
            .layer_count(1);

        let barrier = ash::vk::ImageMemoryBarrier::default()
            .old_layout(old_layout)
            .new_layout(new_layout)
            .src_queue_family_index(ash::vk::QUEUE_FAMILY_IGNORED)
            .dst_queue_family_index(ash::vk::QUEUE_FAMILY_IGNORED)
            .src_access_mask(src_access)
            .dst_access_mask(dst_access)
            .image(self.underlying)
            .subresource_range(sr_range);

        unsafe {
            self.device.get_underlying().cmd_pipeline_barrier(
                cmd_buf.get_underlying(),
                src_stage,
                dst_stage,
                ash::vk::DependencyFlags::empty(),
                &[],
                &[],
                &[barrier],
            );
        }
    }
}

pub struct ImageView<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::ImageView,
    size: Vector2u,
}

impl<'ctx> Wrapper for ImageView<'ctx> {
    type Underlying = ash::vk::ImageView;

    unsafe fn get_underlying(&self) -> ash::vk::ImageView {
        self.underlying
    }
}

impl<'ctx> ImageView<'ctx> {
    pub fn create(
        device: &'ctx vk::Device<'ctx>,
        image: &Image<'ctx>,
        format: vk::Format,
        aspect_mask: vk::ImageAspectFlags
    ) -> Result<Self, String> {
        unsafe {
            Self::create_from_vk_image(
                device,
                image.get_underlying(),
                image.size,
                format,
                aspect_mask,
            )
        }
    }

    pub(crate) unsafe fn create_from_vk_image(
        device: &'ctx vk::Device<'ctx>,
        vk_image: ash::vk::Image,
        size: Vector2u,
        format: vk::Format,
        aspect_mask: vk::ImageAspectFlags
    ) -> Result<Self, String> {
        let components = ash::vk::ComponentMapping::default()
            .r(ash::vk::ComponentSwizzle::IDENTITY)
            .g(ash::vk::ComponentSwizzle::IDENTITY)
            .b(ash::vk::ComponentSwizzle::IDENTITY)
            .a(ash::vk::ComponentSwizzle::IDENTITY);
        let sr_range = ash::vk::ImageSubresourceRange::default()
            .aspect_mask(aspect_mask)
            .base_mip_level(0)
            .level_count(1)
            .base_array_layer(0)
            .layer_count(1);

        let view_info = ash::vk::ImageViewCreateInfo::default()
            .image(vk_image)
            .view_type(ash::vk::ImageViewType::TYPE_2D)
            .format(format)
            .components(components)
            .subresource_range(sr_range);

        let view = unsafe {
            device.get_underlying().create_image_view(&view_info, None)
        }
            .map_err(|err| err.to_string())?;
        Ok(Self {
            device,
            underlying: view,
            size,
        })
    }

    pub fn destroy(self) {
        unsafe { self.device.get_underlying().destroy_image_view(self.underlying, None); }
    }

    pub fn get_size(&self) -> &Vector2u {
        &self.size
    }
}

impl<'ctx> Drop for Image<'ctx> {
    fn drop(&mut self) {
        if cfg!(debug_assertions) && !self.is_destroyed {
            panic!("Vulkan image was dropped without being destroyed!")
        }
    }
}

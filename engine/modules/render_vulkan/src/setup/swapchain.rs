use std::sync::MutexGuard;
use ash::vk;
use ash::prelude::VkResult;
use argus_util::math::Vector2u;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::util::*;
use crate::util::defines::MAX_FRAMES_IN_FLIGHT;

#[derive(Default)]
pub(crate) struct SwapchainSupportInfo {
    pub(crate) caps: vk::SurfaceCapabilitiesKHR,
    pub(crate) formats: Vec<vk::SurfaceFormatKHR>,
    pub(crate) present_modes: Vec<vk::PresentModeKHR>,
}

pub(crate) struct VulkanSwapchain {
    handle: vk::SwapchainKHR,
    pub(crate) resolution: Vector2u,
    surface: vk::SurfaceKHR,
    images: Vec<vk::Image>,
    image_views: Vec<vk::ImageView>,
    pub(crate) framebuffers: Vec<vk::Framebuffer>,
    pub(crate) image_format: vk::Format,
    pub(crate) extent: vk::Extent2D,
    pub(crate) composite_render_pass: vk::RenderPass,

    pub(crate) image_avail_sem: [vk::Semaphore; MAX_FRAMES_IN_FLIGHT],
    pub(crate) render_done_sem: [vk::Semaphore; MAX_FRAMES_IN_FLIGHT],
    pub(crate) in_flight_fence: [vk::Fence; MAX_FRAMES_IN_FLIGHT],
}

impl VulkanSwapchain {
    pub(crate) unsafe fn get_handle(&self) -> vk::SwapchainKHR {
        self.handle
    }
}

pub(crate) unsafe fn query_swapchain_support(
    instance: &VulkanInstance,
    device: vk::PhysicalDevice,
    surface: vk::SurfaceKHR
) -> VkResult<SwapchainSupportInfo> {
    let ext_khr_surface = instance.khr_surface();
    let phys_dev = device;
    let caps =
        ext_khr_surface.get_physical_device_surface_capabilities(phys_dev, surface)?;
    let formats =
        ext_khr_surface.get_physical_device_surface_formats(phys_dev, surface)?;
    let present_modes =
        ext_khr_surface.get_physical_device_surface_present_modes(phys_dev, surface)?;

    Ok(SwapchainSupportInfo {
        caps,
        formats,
        present_modes,
    })
}

fn select_swap_surface_format(support_info: &SwapchainSupportInfo) -> vk::SurfaceFormatKHR {
    for format in &support_info.formats {
        if format.format == vk::Format::B8G8R8A8_SRGB &&
            format.color_space == vk::ColorSpaceKHR::SRGB_NONLINEAR {
            return *format;
        }
    }
    support_info.formats[0]
}

fn select_swap_present_mode(support_info: &SwapchainSupportInfo) -> vk::PresentModeKHR {
    if support_info.present_modes.contains(&vk::PresentModeKHR::MAILBOX) {
        vk::PresentModeKHR::MAILBOX
    } else {
        vk::PresentModeKHR::FIFO
    }
}

fn select_swap_extent(caps: &vk::SurfaceCapabilitiesKHR, resolution: &Vector2u) -> vk::Extent2D {
    if caps.current_extent.width != u32::MAX {
        return caps.current_extent;
    }

    vk::Extent2D::default()
        .width(resolution.x.clamp(caps.min_image_extent.width, caps.max_image_extent.width))
        .height(resolution.y.clamp(caps.min_image_extent.height, caps.max_image_extent.height))
}

pub(crate) fn create_swapchain(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    surface: vk::SurfaceKHR,
    resolution: Vector2u,
) -> Result<VulkanSwapchain, String> {
    let support_info = unsafe {
        query_swapchain_support(instance, device.physical_device, surface)
            .map_err(|err| format!("Failed to query Vulkan swapchain support: {}", err))?
    };
    if support_info.formats.is_empty() {
        return Err("No available swapchain formats".to_owned());
    }
    if support_info.present_modes.is_empty() {
        return Err("No available swapchain present modes".to_owned());
    }

    let format = select_swap_surface_format(&support_info);
    let present_mode = select_swap_present_mode(&support_info);
    let extent = select_swap_extent(&support_info.caps, &resolution);

    let mut image_count = support_info.caps.min_image_count + 1;
    if support_info.caps.max_image_count != 0 && image_count > support_info.caps.max_image_count {
        image_count = support_info.caps.max_image_count;
    }

    let mut sc_create_info = vk::SwapchainCreateInfoKHR::default()
        .surface(surface)
        .min_image_count(image_count)
        .image_format(format.format)
        .image_color_space(format.color_space)
        .image_extent(extent)
        .image_array_layers(1)
        .image_usage(vk::ImageUsageFlags::COLOR_ATTACHMENT);

    let queue_indices = [device.queue_indices.graphics_family, device.queue_indices.present_family];

    if device.queue_indices.graphics_family == device.queue_indices.present_family {
        sc_create_info = sc_create_info
            .image_sharing_mode(vk::SharingMode::EXCLUSIVE)
            .queue_family_indices(&[]);
    } else {
        sc_create_info = sc_create_info
            .image_sharing_mode(vk::SharingMode::CONCURRENT)
            .queue_family_indices(&queue_indices);
    }

    sc_create_info = sc_create_info
        .pre_transform(support_info.caps.current_transform)
        .composite_alpha(vk::CompositeAlphaFlagsKHR::OPAQUE)
        .present_mode(present_mode)
        .clipped(true)
        .old_swapchain(vk::SwapchainKHR::null());

    let sc_device = &device.ext_khr_swapchain;

    let handle = unsafe {
        sc_device.create_swapchain(&sc_create_info, None)
        .map_err(|err| format!("Failed to create Vulkan swapchain: {}", err))?
    };

    // need to create the render pass before we create the framebuffers
    let composite_render_pass = create_render_pass(
        device,
        format.format,
        vk::ImageLayout::PRESENT_SRC_KHR,
        false,
    )
        .map_err(|err| format!("Failed to create render pass: {}", err))?;

    let images = unsafe {
        sc_device.get_swapchain_images(handle)
            .map_err(|err| format!("Failed to get swapchain images: {}", err))?
    };

    let mut image_views: Vec<vk::ImageView> = Vec::new();
    let mut framebuffers: Vec<vk::Framebuffer> = Vec::new();

    for sc_image in &images {
        let vk_image_view = unsafe {
            create_vk_image_view(device, *sc_image, format.format, vk::ImageAspectFlags::COLOR)?
        };
        image_views.push(vk_image_view);

        //auto light_opac_image = create_image_and_image_view(device, VK_FORMAT_R32_SFLOAT,
        //        { extent.width, extent.height }, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        let framebuffer = create_framebuffer_from_views(
            device,
            composite_render_pass,
            &[vk_image_view],
            Vector2u::new(extent.width, extent.height),
        )?;
        framebuffers.push(framebuffer);
    }

    let mut image_avail_sem: [vk::Semaphore; MAX_FRAMES_IN_FLIGHT] = Default::default();
    let mut render_done_sem: [vk::Semaphore; MAX_FRAMES_IN_FLIGHT] = Default::default();
    let mut in_flight_fence: [vk::Fence; MAX_FRAMES_IN_FLIGHT] = Default::default();
    for i in 0..MAX_FRAMES_IN_FLIGHT {
        let sem_info = vk::SemaphoreCreateInfo::default();
        image_avail_sem[i] = unsafe {
            device.logical_device.create_semaphore(&sem_info, None)
                .map_err(|err| format!("Failed to create swapchain semaphores: {}", err))?
        };
        render_done_sem[i] = unsafe {
            device.logical_device.create_semaphore(&sem_info, None)
                .map_err(|err| format!("Failed to create swapchain semaphores: {}", err))?
        };

        let fence_info = vk::FenceCreateInfo::default().flags(vk::FenceCreateFlags::SIGNALED);
        in_flight_fence[i] = unsafe {
            device.logical_device.create_fence(&fence_info, None)
                .map_err(|err| format!("Failed to create swapchain fences: {}", err))?
        };
    }

    let sc_info = VulkanSwapchain {
        handle,
        resolution,
        surface,
        images,
        image_views,
        framebuffers,
        image_format: format.format,
        extent,
        composite_render_pass,
        image_avail_sem,
        render_done_sem,
        in_flight_fence,
    };

    Ok(sc_info)
}

pub(crate) unsafe fn recreate_swapchain(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    swapchain: VulkanSwapchain,
    new_resolution: Vector2u,
    _submit_lock: MutexGuard<()>,
    _gfx_lock: MutexGuard<()>,
    _present_lock: Option<MutexGuard<()>>,
) -> Result<VulkanSwapchain, String> {
    let surface = swapchain.surface;

    device.logical_device.device_wait_idle()
        .map_err(|err| format!("vkDeviceWaitIdle failed: {}", err))?;


    destroy_swapchain(device, swapchain)
        .map_err(|err| format!("Failed to destroy swapchain: {}", err))?;

    create_swapchain(instance, device, surface, new_resolution)
}

pub(crate) unsafe fn destroy_swapchain(
    device: &VulkanDevice,
    swapchain: VulkanSwapchain,
) -> VkResult<()> {
    for i in 0..MAX_FRAMES_IN_FLIGHT {
        //device.logical_device.wait_for_fences(&[swapchain.in_flight_fence[i]], true, u64::MAX)?;

        device.logical_device.destroy_semaphore(swapchain.image_avail_sem[i], None);
        device.logical_device.destroy_semaphore(swapchain.render_done_sem[i], None);

        device.logical_device.destroy_fence(swapchain.in_flight_fence[i], None);
    }

    for fb in swapchain.framebuffers {
        destroy_framebuffer(device, fb);
    }

    for vk_image_view in swapchain.image_views {
        destroy_vk_image_view(device, vk_image_view);
    }

    destroy_render_pass(device, swapchain.composite_render_pass);

    device.ext_khr_swapchain.destroy_swapchain(swapchain.handle, None);

    Ok(())
}

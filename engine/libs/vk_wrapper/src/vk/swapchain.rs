use std::{array, iter};
use std::sync::MutexGuard;
use ash::vk::Handle;
use argus_util::math::Vector2u;
use crate::vk;
use crate::vk::Wrapper;

#[derive(Default)]
pub struct SwapchainSupportInfo {
    pub caps: ash::vk::SurfaceCapabilitiesKHR,
    pub formats: Vec<ash::vk::SurfaceFormatKHR>,
    pub present_modes: Vec<ash::vk::PresentModeKHR>,
}

pub struct Swapchain<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::SwapchainKHR,
    pub resolution: Vector2u,
    surface: vk::Surface<'ctx>,
    pub framebuffers: Vec<vk::Framebuffer<'ctx>>,
    pub image_format: vk::Format,
    pub extent: vk::Extent2D,
    pub(crate) color_att_loc: u32,
    pub composite_render_pass: vk::RenderPass<'ctx>,

    pub image_avail_sem: [vk::Semaphore<'ctx>; vk::MAX_FRAMES_IN_FLIGHT],
    pub render_done_sem: Vec<vk::Semaphore<'ctx>>,
    pub in_flight_fence: [vk::Fence<'ctx>; vk::MAX_FRAMES_IN_FLIGHT],
}

impl<'ctx> Wrapper for Swapchain<'ctx> {
    type Underlying = ash::vk::SwapchainKHR;

    unsafe fn get_underlying(&self) -> ash::vk::SwapchainKHR {
        self.underlying
    }
}

impl<'ctx> Swapchain<'ctx> {
    pub unsafe fn get_handle(&self) -> u64 {
        self.underlying.as_raw()
    }

    pub fn get_device(&self) -> &'ctx vk::Device<'ctx> {
        self.device
    }

    pub fn acquire_next_image(&self, frame: usize, timeout: u64) -> Result<(u32, bool), String> {
        unsafe {
            self.device.khr_swapchain().acquire_next_image(
                self.get_underlying(),
                timeout,
                self.image_avail_sem[frame].get_underlying(),
                ash::vk::Fence::null(),
            )
        }
            .map_err(|err| format!("vkAcquireNextImageKHR failed: {err}"))
    }

    pub fn create(
        device: &'ctx vk::Device<'ctx>,
        surface: vk::Surface<'ctx>,
        resolution: Vector2u,
        color_att_loc: u32,
    ) -> Result<Swapchain<'ctx>, String> {
        assert_eq!(device.get_instance(), surface.get_instance());

        let support_info = query_swapchain_support(&device.physical_device, &surface)
            .map_err(|err| format!("Failed to query Vulkan swapchain support: {err}"))?;
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

        let mut sc_create_info = ash::vk::SwapchainCreateInfoKHR::default()
            .surface(unsafe { surface.get_underlying() })
            .min_image_count(image_count)
            .image_format(format.format)
            .image_color_space(format.color_space)
            .image_extent(extent)
            .image_array_layers(1)
            .image_usage(vk::ImageUsageFlags::COLOR_ATTACHMENT);

        let queue_indices = [device.queue_indices.graphics_family, device.queue_indices.present_family];

        if device.queue_indices.graphics_family == device.queue_indices.present_family {
            sc_create_info = sc_create_info
                .image_sharing_mode(ash::vk::SharingMode::EXCLUSIVE)
                .queue_family_indices(&[]);
        } else {
            sc_create_info = sc_create_info
                .image_sharing_mode(ash::vk::SharingMode::CONCURRENT)
                .queue_family_indices(&queue_indices);
        }

        sc_create_info = sc_create_info
            .pre_transform(support_info.caps.current_transform)
            .composite_alpha(ash::vk::CompositeAlphaFlagsKHR::OPAQUE)
            .present_mode(present_mode)
            .clipped(true)
            .old_swapchain(ash::vk::SwapchainKHR::null());

        let sc_device = &device.ext_khr_swapchain;

        let handle = unsafe {
            sc_device.create_swapchain(&sc_create_info, None)
                .map_err(|err| format!("Failed to create Vulkan swapchain: {}", err))?
        };

        // need to create the render pass before we create the swapchain framebuffers
        let composite_render_pass = vk::RenderPass::create_basic(
            device,
            format.format,
            vk::ImageLayout::PRESENT_SRC_KHR,
            color_att_loc,
            None,
        )
            .map_err(|err| format!("Failed to create render pass: {}", err))?;

        let framebuffers = unsafe {
            vk::Framebuffer::create_from_swapchain_images(
                device,
                &composite_render_pass,
                handle,
                resolution,
                format,
            )?
        };

        let image_avail_sem: [vk::Semaphore; vk::MAX_FRAMES_IN_FLIGHT] =
            array::from_fn(
                |_| vk::Semaphore::create(device, &vk::SemaphoreCreateInfo::default())
                    .map_err(|err| format!("Failed toc reate swapchain semaphores: {}", err))
            )
                .try_map(|s| s)?;
        let render_done_sem: Vec<vk::Semaphore> =
            iter::repeat_with(
                || vk::Semaphore::create(device, &vk::SemaphoreCreateInfo::default())
                    .map_err(|err| format!("Failed to create swapchain semaphores: {}", err))
            )
                .take(framebuffers.len())
                .collect::<Result<Vec<_>, _>>()?;

        let in_flight_fence: [vk::Fence; vk::MAX_FRAMES_IN_FLIGHT] =
            array::from_fn(
                |_| vk::Fence::create(
                    device,
                    &vk::FenceCreateInfo::default().flags(vk::FenceCreateFlags::SIGNALED)
                )
                    .map_err(|err| format!("Failed to create swapchain fences: {}", err))
            )
                .try_map(|f| f)?;

        let sc_info = Swapchain {
            device,
            underlying: handle,
            resolution,
            surface,
            framebuffers,
            image_format: format.format,
            extent,
            color_att_loc,
            composite_render_pass,
            image_avail_sem,
            render_done_sem,
            in_flight_fence,
        };

        Ok(sc_info)
    }

    pub fn recreate(
        self,
        new_resolution: Vector2u,
        _submit_lock: MutexGuard<()>,
        _gfx_lock: MutexGuard<()>,
        _present_lock: Option<MutexGuard<()>>,
    ) -> Result<Swapchain<'ctx>, String> {
        let device = self.device;

        device.wait_idle()
            .map_err(|err| format!("vkDeviceWaitIdle failed: {}", err))?;

        let color_att_loc = self.color_att_loc;

        let surface = self.destroy()
            .map_err(|err| format!("Failed to destroy swapchain: {}", err))?;

        Self::create(device, surface, new_resolution, color_att_loc)
    }

    pub fn destroy(
        self,
    ) -> Result<vk::Surface<'ctx>, String> {
        for i in 0..vk::MAX_FRAMES_IN_FLIGHT {
            self.image_avail_sem[i].destroy();

            self.in_flight_fence[i].destroy();
        }

        for sem in self.render_done_sem {
            sem.destroy();
        }

        for fb in self.framebuffers {
            fb.destroy();
        }

        self.composite_render_pass.destroy();

        unsafe { self.device.ext_khr_swapchain.destroy_swapchain(self.underlying, None); }

        Ok(self.surface)
    }
}

pub fn query_swapchain_support(
    device: &vk::PhysicalDevice,
    surface: &vk::Surface
) -> Result<SwapchainSupportInfo, String> {
    let caps = surface.get_physical_device_capabilities(device)?;
    let formats = surface.get_physical_device_formats(device)?;
    let present_modes = surface.get_physical_device_present_modes(device)?;

    Ok(SwapchainSupportInfo {
        caps,
        formats,
        present_modes,
    })
}

fn select_swap_surface_format(support_info: &SwapchainSupportInfo) -> ash::vk::SurfaceFormatKHR {
    for format in &support_info.formats {
        if format.format == ash::vk::Format::B8G8R8A8_SRGB &&
            format.color_space == ash::vk::ColorSpaceKHR::SRGB_NONLINEAR {
            return *format;
        }
    }
    support_info.formats[0]
}

fn select_swap_present_mode(support_info: &SwapchainSupportInfo) -> ash::vk::PresentModeKHR {
    if support_info.present_modes.contains(&ash::vk::PresentModeKHR::MAILBOX) {
        ash::vk::PresentModeKHR::MAILBOX
    } else {
        ash::vk::PresentModeKHR::FIFO
    }
}

fn select_swap_extent(caps: &ash::vk::SurfaceCapabilitiesKHR, resolution: &Vector2u)
    -> vk::Extent2D {
    if caps.current_extent.width != u32::MAX {
        return caps.current_extent;
    }

    vk::Extent2D::default()
        .width(resolution.x.clamp(caps.min_image_extent.width, caps.max_image_extent.width))
        .height(resolution.y.clamp(caps.min_image_extent.height, caps.max_image_extent.height))
}

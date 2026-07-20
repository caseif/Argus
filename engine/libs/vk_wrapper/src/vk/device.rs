use std::ffi::CStr;
use std::result::Result;
use ash::khr;
use ash::prelude::VkResult;
use argus_logging::{debug, warn, Logger};
use crate::vk;

pub use ash::vk::PhysicalDeviceFeatures;
pub use ash::vk::PhysicalDeviceProperties;
use crate::vk::Wrapper;

const DISCRETE_GPU_RATING_BONUS: u64 = 10000;

pub const ENGINE_DEVICE_EXTENSIONS: &[&CStr] = &[
    khr::swapchain::NAME,
    khr::maintenance1::NAME,
];

pub struct Device<'inst> {
    #[allow(dead_code)]
    pub instance: &'inst vk::Instance,
    pub underlying: ash::Device,
    pub physical_device: PhysicalDevice<'inst>,
    pub ext_khr_swapchain: khr::swapchain::Device,
    pub queue_indices: vk::QueueFamilyIndices,
    pub queues: vk::QueueFamilies<'inst>,
    pub queue_mutexes: vk::QueueMutexes,
    pub limits: ash::vk::PhysicalDeviceLimits,
}

impl<'inst> Device<'inst> {
    pub fn get_instance(&self) -> &vk::Instance {
        self.instance
    }

    pub unsafe fn get_underlying(&self) -> &ash::Device {
        &self.underlying
    }

    pub fn khr_swapchain(&self) -> &khr::swapchain::Device {
        &self.ext_khr_swapchain
    }

    #[allow(dead_code)]
    pub fn destroy(self) {
        unsafe { self.underlying.destroy_device(None); }
    }

    pub(crate) fn create_vk_image(&self, create_info: &ash::vk::ImageCreateInfo)
        -> Result<ash::vk::Image, String> {
        unsafe { self.underlying.create_image(create_info, None) }
            .map_err(|err| err.to_string())
    }

    pub fn wait_idle(&self) -> Result<(), String> {
        unsafe { self.underlying.device_wait_idle() }
            .map_err(|err| err.to_string())
    }

    pub fn wait_for_fences(&self, fences: &[&vk::Fence<'_>], wait_all: bool, timeout: u64)
                           -> Result<(), String> {
        unsafe {
            self.underlying.wait_for_fences(
                &fences.iter().map(|fence| fence.get_underlying()).collect::<Vec<_>>(),
                wait_all,
                timeout,
            )
                .map_err(|err| err.to_string())
        }
    }

    pub fn reset_fences(&self, fences: &[&vk::Fence<'_>]) -> Result<(), String> {
        unsafe {
            self.underlying.reset_fences(
                &fences.iter().map(|fence| fence.get_underlying()).collect::<Vec<_>>(),
            )
                .map_err(|err| err.to_string())
        }
    }

    pub fn update_descriptor_sets(&self, write_desc_sets: Vec<vk::WriteDescriptorSet>) {
        let vk_write_desc_sets: Vec<_> =
            write_desc_sets.into_iter().map(|wds| wds.prepare()).collect();

        unsafe {
            self.underlying.update_descriptor_sets(
                &vk_write_desc_sets.iter().map(|wds| wds.create_underlying()).collect::<Vec<_>>(),
                &[],
            );
        }
    }
}

fn is_device_suitable(
    device: &PhysicalDevice,
    probe_surface: &vk::Surface,
    logger: &Logger,
) -> bool {
    let enum_ext_res = device.enumerate_extension_properties();
    let available_exts = match enum_ext_res {
        Ok(exts) => exts,
        Err(err) => {
            warn!(logger, "Failed to enumerate device extension properties: {:?}", err);
            return false;
        }
    };
    let available_ext_names: Vec<_> = available_exts.iter()
        .filter_map(|ext| {
            match ext.extension_name_as_c_str() {
                Ok(name) => Some(name),
                Err(_) => {
                    warn!(
                        logger,
                        "Encountered invalid (corrupted?) Vulkan extension name: {:?}",
                        ext.extension_name,
                    );
                    None
                }
            }
        })
        .collect();

    let required_exts = ENGINE_DEVICE_EXTENSIONS;

    for ext in required_exts {
        if !available_ext_names.iter().any(|name| name == ext) {
            debug!(
                logger,
                "Physical device '{:?}' is not suitable (missing required extensions)",
                device,
            );
            return false;
        }
    }

    let sc_support =
        match vk::query_swapchain_support(device, probe_surface) {
            Ok(support) => support,
            Err(err) => {
                warn!(
                    logger,
                    "Unable to query swapchain support for Vulkan device: {}",
                    err.to_string(),
                );
                return false;
            }
        };
    if sc_support.formats.is_empty() {
        debug!(
            logger,
            "Physical device '{:?}' is not suitable (no available swapchain formats)",
            device,
        );
        return false;
    }

    if sc_support.present_modes.is_empty() {
        debug!(
            logger,
            "Physical device '{:?}' is not suitable (no available swapchain present modes)",
            device,
        );
        return false;
    }

    true
}

fn rate_physical_device(
    device: &PhysicalDevice,
    logger: &Logger,
) -> Option<u64> {
    let mut score: u64 = 0;

    // SAFETY: In principle this is safe so long as the underlying function in
    // the Vulkan driver is sound.
    let props = device.get_properties();
    let features = device.get_features();

    if features.independent_blend == 0 {
        warn!(logger, "Cannot use device {:?} which does not support independentBlend", device);
        return None;
    }

    if props.device_type == ash::vk::PhysicalDeviceType::DISCRETE_GPU {
        score += DISCRETE_GPU_RATING_BONUS;
    }

    score += props.limits.max_image_dimension2_d as u64;

    //TODO: do some more checks at some point

    Some(score)
}

pub fn select_physical_device<'inst>(
    instance: &'inst vk::Instance,
    probe_surface: &vk::Surface,
    logger: &Logger,
) -> Result<PhysicalDevice<'inst>, String> {
    let devices = instance.enumerate_physical_devices()?;

    if devices.is_empty() {
        return Err("No physical video devices found".to_owned());
    }

    let mut best_device: Option<PhysicalDevice> = None;
    let mut best_rating: u64 = 0;

    for device in devices {
        let dev_props = device.get_properties();
        let dev_name = dev_props.device_name_as_c_str()
            .map_err(|err| err.to_string())?
            .to_string_lossy();

        debug!(logger, "Considering physical device '{}'", dev_name);

        if device.get_queue_family_indices(probe_surface).is_err() {
            debug!(logger, "Device {} is not suitable (unable to probe queue families)", dev_name);
            continue;
        };

        if !is_device_suitable(&device, probe_surface, logger) {
            continue;
        }

        let Some(rating) = rate_physical_device(&device, logger) else {
            continue;
        };

        debug!(logger, "Physical device '{}' was assigned rating of {}", dev_name, rating);
        if rating > best_rating {
            best_device = Some(device);
            best_rating = rating;
        }
    }

    match best_device {
        Some(dev) => Ok(dev),
        None => Err("Failed to find suitable video device".to_owned()),
    }
}

#[derive(Debug)]
pub struct PhysicalDevice<'inst> {
    instance: &'inst vk::Instance,
    underlying: ash::vk::PhysicalDevice,
}

impl<'inst> Wrapper for PhysicalDevice<'inst> {
    type Underlying = ash::vk::PhysicalDevice;

    unsafe fn get_underlying(&self) -> ash::vk::PhysicalDevice {
        self.underlying
    }
}

impl<'inst> PhysicalDevice<'inst> {
    pub(crate) fn new(instance: &'inst vk::Instance, device: ash::vk::PhysicalDevice) -> Self {
        Self {
            instance,
            underlying: device,
        }
    }

    pub fn get_instance(&self) -> &vk::Instance {
        self.instance
    }

    pub fn get_properties(&self) -> vk::PhysicalDeviceProperties {
        unsafe { self.instance.get_underlying().get_physical_device_properties(self.underlying) }
    }

    pub fn get_features(&self) -> vk::PhysicalDeviceFeatures {
        unsafe { self.instance.get_underlying().get_physical_device_features(self.underlying) }
    }

    fn get_queue_family_properties(&self) -> Vec<ash::vk::QueueFamilyProperties> {
        unsafe {
            self.instance.get_underlying()
                .get_physical_device_queue_family_properties(self.underlying)
        }
    }

    pub fn get_queue_family_indices(
        &self,
        surface: &vk::Surface,
    ) -> Result<vk::QueueFamilyIndices, String> {
        assert_eq!(self.instance, surface.get_instance());

        let mut graphics_family: Option<u32> = None;
        let mut transfer_family: Option<u32> = None;
        let mut present_family: Option<u32> = None;

        let queue_families = self.get_queue_family_properties();

        for i in 0..queue_families.len() {
            let queue_family = &(*queue_families)[i];
            if transfer_family.is_none() &&
                queue_family.queue_flags.contains(ash::vk::QueueFlags::TRANSFER) &&
                !queue_family.queue_flags.contains(ash::vk::QueueFlags::GRAPHICS) {
                transfer_family = Some(i as u32);
            }

            // check if queue family is suitable for graphics
            if graphics_family.is_none() &&
                queue_family.queue_flags.contains(ash::vk::QueueFlags::GRAPHICS) {
                graphics_family = Some(i as u32);
            }

            let device_has_present_support = surface.get_physical_device_support(self, i as u32)
                .map_err(|err| err.to_string())?;
            if device_has_present_support {
                present_family = Some(i as u32);
            }
        }

        let Some(graphics_family) = graphics_family else {
            return Err("Device does not support any queue families with VK_QUEUE_VIDEO_BIT".to_owned());
        };

        let Some(present_family) = present_family else {
            return Err(
                "Device does not support any queue families that can present to a surface".to_owned()
            );
        };

        let indices = vk::QueueFamilyIndices {
            graphics_family,
            transfer_family: transfer_family.unwrap_or(graphics_family),
            present_family,
        };

        Ok(indices)
    }

    fn enumerate_extension_properties(&self) -> VkResult<Vec<ash::vk::ExtensionProperties>>{
        unsafe {
            self.instance.get_underlying().enumerate_device_extension_properties(self.underlying)
        }
    }

    pub(crate) fn get_memory_properties(&self) -> ash::vk::PhysicalDeviceMemoryProperties {
        unsafe {
            self.instance.get_underlying().get_physical_device_memory_properties(self.underlying)
        }
    }
}

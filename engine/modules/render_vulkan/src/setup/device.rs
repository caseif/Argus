use std::collections::HashSet;
use std::ffi::CStr;
use std::result::Result;
use ash::{khr, vk};
use argus_logging::{debug, info, warn};
use crate::setup::instance::VulkanInstance;
use crate::setup::LOGGER;
use crate::setup::queues::*;
use crate::setup::swapchain::query_swapchain_support;

const DISCRETE_GPU_RATING_BONUS: u64 = 10000;

pub(crate) const ENGINE_DEVICE_EXTENSIONS: &[&CStr] = &[
    khr::swapchain::NAME,
    khr::maintenance1::NAME,
];

#[derive(Clone)]
pub(crate) struct VulkanDevice {
    pub(crate) physical_device: vk::PhysicalDevice,
    pub(crate) logical_device: ash::Device,
    pub(crate) ext_khr_swapchain: khr::swapchain::Device,
    pub(crate) queue_indices: QueueFamilyIndices,
    pub(crate) queues: QueueFamilies,
    pub(crate) queue_mutexes: QueueMutexes,
    pub(crate) limits: vk::PhysicalDeviceLimits,
}

impl VulkanDevice {
    pub(crate) fn new(
        instance: &VulkanInstance,
        probe_surface: vk::SurfaceKHR
    ) -> Result<VulkanDevice, String> {
        let (phys_device, qf_indices) = select_physical_device(instance, probe_surface)
            .map_err(|err| err.to_string())?;

        let phys_dev_props = unsafe {
            instance.get_underlying().get_physical_device_properties(phys_device)
        };

        info!(
            LOGGER,
            "Selected video device {}",
            phys_dev_props.device_name_as_c_str().map(|s| s.to_string_lossy().to_string())
                .unwrap_or_else(|_| "(unknown)".to_owned()),
        );

        let unique_queue_families = HashSet::from([
            qf_indices.graphics_family,
            qf_indices.present_family,
            qf_indices.transfer_family,
        ]);
        let mut queue_create_infos = Vec::with_capacity(unique_queue_families.len());
        for queue_id in unique_queue_families {
            let queue_create_info = vk::DeviceQueueCreateInfo::default()
                .queue_family_index(queue_id)
                .queue_priorities(&[1.0]);

            queue_create_infos.push(queue_create_info);
        }

        let dev_features = vk::PhysicalDeviceFeatures::default()
            .independent_blend(true);

        let ext_names: Vec<*const i8> = ENGINE_DEVICE_EXTENSIONS.iter()
            .map(|ext| ext.as_ptr())
            .collect();

        let dev_create_info = vk::DeviceCreateInfo::default()
            .queue_create_infos(queue_create_infos.as_slice())
            .enabled_features(&dev_features)
            .enabled_extension_names(ext_names.as_slice());

        let device = unsafe {
            instance.get_underlying().create_device(phys_device, &dev_create_info, None)
                .map_err(|err| err.to_string())?
        };

        debug!(LOGGER, "Successfully created logical Vulkan device");

        let sc_device = khr::swapchain::Device::new(instance.get_underlying(), &device);

        let graphics_queue = unsafe { device.get_device_queue(qf_indices.graphics_family, 0) };
        let transfer_queue = unsafe { device.get_device_queue(qf_indices.transfer_family, 0) };
        let present_queue = unsafe { device.get_device_queue(qf_indices.present_family, 0) };

        let phys_device_limits = unsafe {
            let props = instance.get_underlying().get_physical_device_properties(phys_device);
            props.limits
        };

        Ok(VulkanDevice {
            physical_device: phys_device,
            logical_device: device,
            ext_khr_swapchain: sc_device,
            queue_indices: qf_indices,
            queues: QueueFamilies {
                graphics_family: graphics_queue,
                transfer_family: transfer_queue,
                present_family: present_queue,
            },
            queue_mutexes: Default::default(),
            limits: phys_device_limits,
        })
    }

    pub(crate) fn khr_swapchain(&self) -> &khr::swapchain::Device {
        &self.ext_khr_swapchain
    }

    pub(crate) fn destroy(self) {
        unsafe { self.logical_device.destroy_device(None); }
    }
}

fn get_queue_family_indices(
    device: vk::PhysicalDevice,
    surface: vk::SurfaceKHR,
    surface_instance: &khr::surface::Instance,
    queue_families: &Vec<vk::QueueFamilyProperties>,
) -> Result<QueueFamilyIndices, String> {
    let mut graphics_family: Option<u32> = None;
    let mut transfer_family: Option<u32> = None;
    let mut present_family: Option<u32> = None;

    let mut i: u32 = 0;
    for queue_family in queue_families {
        if transfer_family.is_none() &&
            queue_family.queue_flags.contains(vk::QueueFlags::TRANSFER) &&
            !queue_family.queue_flags.contains(vk::QueueFlags::GRAPHICS) {
            transfer_family = Some(i);
        }

        // check if queue family is suitable for graphics
        if graphics_family.is_none() &&
            queue_family.queue_flags.contains(vk::QueueFlags::GRAPHICS) {
            graphics_family = Some(i);
        }

        let device_has_present_support = unsafe {
            surface_instance.get_physical_device_surface_support(device, i, surface)
                .map_err(|err| err.to_string())?
        };
        if device_has_present_support {
            present_family = Some(i);
        }

        i += 1;
    }

    let Some(graphics_family) = graphics_family else {
        return Err("Device does not support any queue families with VK_QUEUE_VIDEO_BIT".to_owned());
    };

    let Some(present_family) = present_family else {
        return Err(
            "Device does not support any queue families that can present to a surface".to_owned()
        );
    };

    let indices = QueueFamilyIndices {
        graphics_family,
        transfer_family: transfer_family.unwrap_or(graphics_family),
        present_family,
    };

    Ok(indices)
}

fn is_device_suitable(
    instance: &VulkanInstance,
    device: vk::PhysicalDevice,
    probe_surface: vk::SurfaceKHR,
) -> bool {
    let enum_ext_res = unsafe {
        instance.get_underlying().enumerate_device_extension_properties(device)
    };
    let available_exts = match enum_ext_res {
        Ok(exts) => exts,
        Err(err) => {
            warn!(LOGGER, "Failed to enumerate device extension properties: {:?}", err);
            return false;
        }
    };
    let available_ext_names: Vec<_> = available_exts.iter()
        .filter_map(|ext| {
            match ext.extension_name_as_c_str() {
                Ok(name) => Some(name),
                Err(_) => {
                    warn!(
                        LOGGER,
                        "Encountered invalid (corrupted?) Vulkan extension name: {:?}",
                        ext.extension_name,
                    );
                    return None;
                }
            }
        })
        .collect();

    let required_exts = ENGINE_DEVICE_EXTENSIONS;

    for ext in required_exts {
        if !available_ext_names.iter().any(|name| name == ext) {
            debug!(
                LOGGER,
                "Physical device '{:?}' is not suitable (missing required extensions)",
                device,
            );
            return false;
        }
    }

    let sc_support = match unsafe { query_swapchain_support(instance, device, probe_surface) } {
        Ok(support) => support,
        Err(err) => {
            warn!(
                LOGGER,
                "Unable to query swapchain support for Vulkan device: {}",
                err.to_string(),
            );
            return false;
        }
    };
    if sc_support.formats.is_empty() {
        debug!(
            LOGGER,
            "Physical device '{:?}' is not suitable (no available swapchain formats)",
            device,
        );
        return false;
    }

    if sc_support.present_modes.is_empty() {
        debug!(
            LOGGER,
            "Physical device '{:?}' is not suitable (no available swapchain present modes)",
            device,
        );
        return false;
    }

    true
}

fn rate_physical_device(
    instance: &VulkanInstance,
    device: vk::PhysicalDevice,
    _queue_families: &Vec<vk::QueueFamilyProperties>,
) -> Option<u64> {
    let mut score: u64 = 0;

    // SAFETY: In principle this is safe so long as the underlying function in
    // the Vulkan driver is sound.
    let props = unsafe { instance.get_underlying().get_physical_device_properties(device) };
    let features = unsafe { instance.get_underlying().get_physical_device_features(device) };

    if features.independent_blend == 0 {
        warn!(LOGGER, "Cannot use device {:?} which does not support independentBlend", device);
        return None;
    }

    if props.device_type == vk::PhysicalDeviceType::DISCRETE_GPU {
        score += DISCRETE_GPU_RATING_BONUS;
    }

    score += props.limits.max_image_dimension2_d as u64;

    //TODO: do some more checks at some point

    Some(score)
}

fn select_physical_device(
    instance: &VulkanInstance,
    probe_surface: vk::SurfaceKHR,
) -> Result<(vk::PhysicalDevice, QueueFamilyIndices), String> {
    let devices = unsafe {
        instance.get_underlying().enumerate_physical_devices()
            .map_err(|err| err.to_string())?
    };

    if devices.is_empty() {
        return Err("No physical video devices found".to_owned());
    }

    let mut best_device: Option<(vk::PhysicalDevice, QueueFamilyIndices)> = None;
    let mut best_rating: u64 = 0;

    for device in devices {
        // SAFETY: In principle this is safe so long as the underlying function in
        // the Vulkan driver is sound.
        let dev_props = unsafe {
            instance.get_underlying().get_physical_device_properties(device)
        };
        let dev_name = dev_props.device_name_as_c_str()
            .map_err(|err| err.to_string())?
            .to_string_lossy();

        debug!(LOGGER, "Considering physical device '{}'", dev_name);

        // SAFETY: In principle this is safe so long as the underlying function in
        // the Vulkan driver is sound.
        let queue_families = unsafe {
            instance.get_underlying().get_physical_device_queue_family_properties(device)
        };

        let Ok(indices) = get_queue_family_indices(
            device,
            probe_surface,
            instance.khr_surface(),
            &queue_families,
        ) else {
            debug!(LOGGER, "Device {} is not suitable (unable to probe queue families)", dev_name);
            continue;
        };

        if !is_device_suitable(instance, device, probe_surface) {
            continue;
        }

        let Some(rating) = rate_physical_device(instance, device, &queue_families) else {
            continue;
        };

        debug!(LOGGER, "Physical device '{}' was assigned rating of {}", dev_name, rating);
        if rating > best_rating {
            best_device = Some((device, indices));
            best_rating = rating;
        }
    }

    match best_device {
        Some(dev) => Ok(dev),
        None => Err("Failed to find suitable video device".to_owned()),
    }
}

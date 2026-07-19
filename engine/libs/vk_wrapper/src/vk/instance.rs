use std::collections::HashSet;
use std::ffi::{CStr, CString};
use std::fmt::Debug;
use ash::{ext, khr};
use ash::vk::Handle;
use argus_logging::{debug, warn, Logger};
use crate::vk;
use crate::vk::Wrapper;

pub const ENGINE_INSTANCE_EXTENSIONS: &[&CStr] = &[
    khr::surface::NAME,
    #[cfg(debug_assertions)]
    ext::debug_utils::NAME,
];

pub const ENGINE_LAYERS: &[&str] = &[
    #[cfg(debug_assertions)]
    "VK_LAYER_KHRONOS_validation",
];

pub struct Instance {
    entry: ash::Entry,
    underlying: ash::Instance,
    ext_khr_surface: khr::surface::Instance,
    ext_khr_swapchain: khr::swapchain::Instance,
    ext_ext_debug_utils: ext::debug_utils::Instance,
}

impl Debug for Instance {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        f.debug_struct("VulkanInstance")
            .field("underlying", &self.underlying.handle())
            .finish()
    }
}

impl PartialEq for Instance {
    fn eq(&self, other: &Self) -> bool {
        self.underlying.handle() == other.underlying.handle()
    }
}

impl Instance {
    pub(crate) fn new(
        entry: ash::Entry,
        required_extensions: &[&CStr],
        client_name: impl AsRef<str>,
        logger: &Logger,
    ) -> Result<Self, String> {
        let all_exts: HashSet<&CStr> = required_extensions.into_iter()
            .chain(ENGINE_INSTANCE_EXTENSIONS)
            .copied()
            .collect();

        if !check_required_extensions(&entry, &all_exts, logger) {
            return Err("Required Vulkan extensions are not available".to_owned());
        }

        let mut all_layers = Vec::new();
        if cfg!(debug_assertions) {
            all_layers.extend_from_slice(ENGINE_LAYERS);
        }

        if !check_required_layers(&entry, &all_layers, logger) {
            return Err("Required Vulkan extensions for engine are not available".to_owned());
        }

        let app_name = CString::new(client_name.as_ref()).unwrap();

        let ext_names = all_exts.iter().map(|ext| ext.as_ptr()).collect::<Vec<_>>();
        let layers_c = all_layers.iter().map(|layer| CString::new(*layer).unwrap()).collect::<Vec<_>>();
        let layer_names = layers_c.iter().map(|layer| layer.as_ptr()).collect::<Vec<_>>();

        let app_info = ash::vk::ApplicationInfo::default()
            .api_version(ash::vk::API_VERSION_1_2)
            .engine_name(c"Argus")
            .engine_version(ash::vk::make_api_version(
                0,
                u32::from_str_radix(env!("CARGO_PKG_VERSION_MAJOR"), 10).unwrap(),
                u32::from_str_radix(env!("CARGO_PKG_VERSION_MINOR"), 10).unwrap(),
                u32::from_str_radix(env!("CARGO_PKG_VERSION_PATCH"), 10).unwrap(),
            ))
            .application_name(app_name.as_c_str())
            //TODO: parse out client version into three components
            .application_version(ash::vk::make_api_version(0, 1, 0, 0));

        let create_info = ash::vk::InstanceCreateInfo::default()
            .application_info(&app_info)
            .enabled_extension_names(&ext_names)
            .enabled_layer_names(&layer_names);

        let handle = unsafe {
            entry.create_instance(&create_info, None)
                .map_err(|e| format!("Failed to create Vulkan instance: {}", e))?
        };

        let ext_khr_surface = khr::surface::Instance::new(&entry, &handle);
        let ext_khr_swapchain = khr::swapchain::Instance::new(&entry, &handle);
        let ext_ext_debug_utils = ext::debug_utils::Instance::new(&entry, &handle);
        
        Ok(Self {
            entry,
            underlying: handle,
            ext_khr_surface,
            ext_khr_swapchain,
            ext_ext_debug_utils,
        })
    }

    pub fn load(
        required_extensions: &[&CStr],
        client_name: impl AsRef<str>,
        logger: &Logger,
    ) -> Result<Instance, String> {
        let ash_entry = match unsafe { ash::Entry::load() } {
            Ok(entry) => entry,
            Err(err) => {
                return Err(format!(
                    "Failed to load Vulkan library: {}",
                    err,
                ));
            },
        };
        Self::new(ash_entry, required_extensions, client_name, logger)
    }

    #[allow(dead_code)]
    pub fn get_entry(&self) -> &ash::Entry {
        &self.entry
    }

    #[allow(dead_code)]
    pub fn get_entry_mut(&mut self) -> &mut ash::Entry {
        &mut self.entry
    }

    pub fn get_underlying(&self) -> &ash::Instance {
        &self.underlying
    }

    pub unsafe fn get_handle(&self) -> u64 {
        self.underlying.handle().as_raw()
    }

    pub fn khr_surface(&self) -> &khr::surface::Instance {
        &self.ext_khr_surface
    }

    #[allow(dead_code)]
    pub fn khr_swapchain(&self) -> &khr::swapchain::Instance {
        &self.ext_khr_swapchain
    }

    pub fn ext_debug_utils(&self) -> &ext::debug_utils::Instance {
        &self.ext_ext_debug_utils
    }

    pub fn enumerate_physical_devices(&'_ self)
                                      -> Result<Vec<vk::PhysicalDevice<'_>>, String> {
        let devices = unsafe {
            self.underlying.enumerate_physical_devices()
                .map_err(|err| err.to_string())?
        };
        Ok(devices.into_iter().map(|dev| vk::PhysicalDevice::new(self, dev)).collect())
    }

    pub fn create_device<'inst>(
        &'inst self,
        physical_device: vk::PhysicalDevice<'inst>,
        probe_surface: &vk::Surface<'inst>,
        logger: &Logger,
    ) -> Result<vk::Device<'inst>, String> {
        assert_eq!(physical_device.get_instance(), self);

        let qf_indices = physical_device.get_queue_family_indices(probe_surface)?;

        let unique_queue_families = HashSet::from([
            qf_indices.graphics_family,
            qf_indices.present_family,
            qf_indices.transfer_family,
        ]);
        let mut queue_create_infos = Vec::with_capacity(unique_queue_families.len());
        for queue_id in unique_queue_families {
            let queue_create_info = ash::vk::DeviceQueueCreateInfo::default()
                .queue_family_index(queue_id)
                .queue_priorities(&[1.0]);

            queue_create_infos.push(queue_create_info);
        }

        let dev_features = vk::PhysicalDeviceFeatures::default()
            .independent_blend(true);

        let ext_names: Vec<_> = vk::ENGINE_DEVICE_EXTENSIONS.iter()
            .map(|ext| ext.as_ptr())
            .collect();

        let dev_create_info = ash::vk::DeviceCreateInfo::default()
            .queue_create_infos(queue_create_infos.as_slice())
            .enabled_features(&dev_features)
            .enabled_extension_names(ext_names.as_slice());

        let logical_device = unsafe {
            self.get_underlying()
                .create_device(physical_device.get_underlying(), &dev_create_info, None)
                .map_err(|err| err.to_string())?
        };

        debug!(logger, "Successfully created logical Vulkan device");

        let sc_device = khr::swapchain::Device::new(&self.underlying, &logical_device);

        let graphics_queue = unsafe {
            vk::Queue::from_underlying(
                &logical_device,
                logical_device.get_device_queue(qf_indices.graphics_family, 0),
            )
        };
        let transfer_queue = unsafe {
            vk::Queue::from_underlying(
                &logical_device,
                logical_device.get_device_queue(qf_indices.transfer_family, 0),
            )
        };
        let present_queue = unsafe {
            vk::Queue::from_underlying(
                &logical_device,
                logical_device.get_device_queue(qf_indices.present_family, 0),
            )
        };

        let phys_device_limits = physical_device.get_properties().limits;

        Ok(vk::Device {
            instance: self,
            underlying: logical_device,
            physical_device,
            ext_khr_swapchain: sc_device,
            queue_indices: qf_indices,
            queues: vk::QueueFamilies {
                graphics_family: graphics_queue,
                transfer_family: transfer_queue,
                present_family: present_queue,
            },
            queue_mutexes: Default::default(),
            limits: phys_device_limits,
        })
    }
}

fn check_required_extensions(entry: &ash::Entry, exts: &HashSet<&CStr>, logger: &Logger) -> bool {
    let available_exts = unsafe { entry.enumerate_instance_extension_properties(None) };
    let available_exts = match available_exts {
        Ok(exts) => exts,
        Err(err) => {
            warn!(logger, "Failed to enumerate available Vulkan extensions: {}", err);
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

    for ext_name in exts {
        if !available_ext_names.iter().any(|name| *name == *ext_name) {
            warn!(
                logger,
                "Required Vulkan extension '{}' is not available",
                ext_name.to_string_lossy(),
            );
            return false;
        }

        debug!(logger, "Found required Vulkan extension {}", ext_name.to_string_lossy());
    }

    true
}

fn check_required_layers(entry: &ash::Entry, layers: &Vec<&str>, logger: &Logger) -> bool {
    if cfg!(debug_assertions) {
        let available_layers = unsafe { entry.enumerate_instance_layer_properties() };
        let available_layers = match available_layers {
            Ok(layers) => layers,
            Err(err) => {
                warn!(logger, "Failed to enumerate available Vulkan layers: {}", err);
                return false;
            }
        };

        let available_layer_names: Vec<_> = available_layers.iter()
            .filter_map(|ext| {
                match ext.layer_name_as_c_str() {
                    Ok(name) => Some(name),
                    Err(_) => {
                        warn!(
                            logger,
                            "Encountered invalid (corrupted?) Vulkan extension name: {:?}",
                            ext.layer_name,
                        );
                        None
                    }
                }
            })
            .collect();

        for layer_name in layers {
            if !available_layer_names.iter().any(|layer| layer.to_str().unwrap() == *layer_name) {
                warn!(logger, "Required Vulkan layer '{}' is not available", layer_name);
                return false;
            }

            debug!(logger, "Found required Vulkan layer {}", layer_name);
        }
    }

    true
}

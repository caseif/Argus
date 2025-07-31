use std::collections::HashSet;
use std::ffi::{CStr, CString};
use ash::{ext, khr, vk, Instance};
use konst::unwrap_ctx;
use konst::primitive::parse_u32;
use argus_core::{ClientConfig, EngineManager};
use argus_logging::{debug, warn};
use argus_wm::{vk_get_required_instance_extensions, Window};
use crate::setup::LOGGER;

pub(crate) const ENGINE_INSTANCE_EXTENSIONS: &[&CStr] = &[
    khr::surface::NAME,
    #[cfg(debug_assertions)]
    ext::debug_utils::NAME,
];

pub(crate) const ENGINE_LAYERS: &[&str] = &[
    //#[cfg(debug_assertions)]
    //"VK_LAYER_KHRONOS_validation",
];

#[derive(Clone)]
pub struct VulkanInstance {
    entry: ash::Entry,
    underlying: Instance,
    ext_khr_surface: khr::surface::Instance,
    ext_khr_swapchain: khr::swapchain::Instance,
    ext_ext_debug_utils: ext::debug_utils::Instance,
}

impl VulkanInstance {
    pub(crate) fn new(entry: ash::Entry, window: &Window, required_extensions: &[&CStr]) -> Result<Self, String> {
        let wm_required_exts: Vec<_> = vk_get_required_instance_extensions(window)?
            .into_iter()
            .map(|ext| CString::new(ext).unwrap())
            .collect();

        let required_exts_c: Vec<_> = required_extensions.iter()
            .map(|s| CString::from(*s))
            .collect();
        
        let all_exts: HashSet<_> = wm_required_exts.iter()
            .map(|ext| ext.as_c_str())
            .chain(required_exts_c.iter().map(|ext| ext.as_c_str()))
            .chain(ENGINE_INSTANCE_EXTENSIONS.iter().copied())
            .collect();

        if !check_required_extensions(&entry, &all_exts) {
            return Err("Required Vulkan extensions are not available".to_owned());
        }

        let mut all_layers = Vec::new();
        if cfg!(debug_assertions) {
            all_layers.extend_from_slice(ENGINE_LAYERS);
        }

        if !check_required_layers(&entry, &all_layers) {
            return Err("Required Vulkan extensions for engine are not available".to_owned());
        }

        let client_name = EngineManager::instance().get_config()
            .get_section::<ClientConfig>().as_ref().unwrap()
            .name.clone();
        let app_name = CString::new(client_name).unwrap();

        let ext_names = all_exts.iter().map(|ext| ext.as_ptr()).collect::<Vec<_>>();
        let layers_c = all_layers.iter().map(|layer| CString::new(*layer).unwrap()).collect::<Vec<_>>();
        let layer_names = layers_c.iter().map(|layer| layer.as_ptr()).collect::<Vec<_>>();

        let app_info = vk::ApplicationInfo::default()
            .api_version(vk::API_VERSION_1_2)
            .engine_name(c"Argus")
            .engine_version(vk::make_api_version(
                0,
                unwrap_ctx!(parse_u32(env!("CARGO_PKG_VERSION_MAJOR"))),
                unwrap_ctx!(parse_u32(env!("CARGO_PKG_VERSION_MINOR"))),
                unwrap_ctx!(parse_u32(env!("CARGO_PKG_VERSION_PATCH"))),
            ))
            .application_name(app_name.as_c_str())
            //TODO: parse out client version into three components
            .application_version(vk::make_api_version(0, 1, 0, 0));

        let create_info = vk::InstanceCreateInfo::default()
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

    pub(crate) fn get_entry(&self) -> &ash::Entry {
        &self.entry
    }

    pub(crate) fn get_entry_mut(&mut self) -> &mut ash::Entry {
        &mut self.entry
    }
    
    pub(crate) fn get_underlying(&self) -> &Instance {
        &self.underlying
    }

    pub(crate) fn khr_surface(&self) -> &khr::surface::Instance {
        &self.ext_khr_surface
    }

    pub(crate) fn khr_swapchain(&self) -> &khr::swapchain::Instance {
        &self.ext_khr_swapchain
    }

    pub(crate) fn ext_debug_utils(&self) -> &ext::debug_utils::Instance {
        &self.ext_ext_debug_utils
    }
}

fn check_required_extensions(entry: &ash::Entry, exts: &HashSet<&CStr>) -> bool {
    let available_exts = unsafe { entry.enumerate_instance_extension_properties(None) };
    let available_exts = match available_exts {
        Ok(exts) => exts,
        Err(err) => {
            warn!(LOGGER, "Failed to enumerate available Vulkan extensions: {}", err);
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
                    None
                }
            }
        })
        .collect();

    for ext_name in exts {
        if !available_ext_names.iter().any(|name| *name == *ext_name) {
            warn!(
                LOGGER,
                "Required Vulkan extension '{}' is not available",
                ext_name.to_string_lossy(),
            );
            return false;
        }

        debug!(LOGGER, "Found required Vulkan extension {}", ext_name.to_string_lossy());
    }

    true
}

fn check_required_layers(entry: &ash::Entry, layers: &Vec<&str>) -> bool {
    if cfg!(debug_assertions) {
        let available_layers = unsafe { entry.enumerate_instance_layer_properties() };
        let available_layers = match available_layers {
            Ok(layers) => layers,
            Err(err) => {
                warn!(LOGGER, "Failed to enumerate available Vulkan layers: {}", err);
                return false;
            }
        };

        let available_layer_names: Vec<_> = available_layers.iter()
            .filter_map(|ext| {
                match ext.layer_name_as_c_str() {
                    Ok(name) => Some(name),
                    Err(_) => {
                        warn!(
                            LOGGER,
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
                warn!(LOGGER, "Required Vulkan layer '{}' is not available", layer_name);
                return false;
            }

            debug!(LOGGER, "Found required Vulkan layer {}", layer_name);
        }
    }

    true
}

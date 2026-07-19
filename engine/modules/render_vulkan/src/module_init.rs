use std::cell::RefCell;
use std::collections::HashMap;
use std::ffi::CString;
use std::sync::atomic::AtomicBool;
use std::sync::{atomic, OnceLock};
use std::time::Duration;
use argus_core::{register_event_handler, register_module, ClientConfig, EngineManager, LifecycleStage, Ordering, TargetThread};
use argus_logging::{crate_logger, debug, info, warn, LogLevel};
use argus_render::common::register_render_backend;
use argus_render::constants::{RESOURCE_TYPE_SHADER_GLSL_FRAG, RESOURCE_TYPE_SHADER_GLSL_VERT};
use argus_resman::{ResourceManager};
use argus_wm::{vk_create_surface, vk_get_required_instance_extensions, vk_is_supported, WindowCreationFlags, WindowEvent, WindowEventType, WindowManager};
use vk_wrapper::vk;
use crate::loader::ShaderLoader;
use crate::renderer::VulkanRenderer;
use crate::resources::RESOURCES_PACK;
use crate::LOGGER;

const BACKEND_ID: &str = "vulkan";

crate_logger!(LOGGER_VK, "vulkan");

thread_local! {
    static RENDERERS: RefCell<HashMap<String, VulkanRenderer<'static, 'static>>> =
        RefCell::new(HashMap::new());
}

static IS_BACKEND_ACTIVE: AtomicBool = AtomicBool::new(false);

static VK_INSTANCE: OnceLock<vk::Instance> = OnceLock::new();
static VK_DEVICE: OnceLock<vk::Device> = OnceLock::new();
static VK_DEBUG_MESSENGER: OnceLock<Option<vk::DebugUtilsMessenger>> = OnceLock::new();

fn debug_callback(
    severity: vk::DebugUtilsMessageSeverityFlagsEXT,
    _ty: vk::DebugUtilsMessageTypeFlagsEXT,
    message: &str,
) -> u32 {
    let (level, _) = match severity {
        vk::DebugUtilsMessageSeverityFlagsEXT::ERROR => (LogLevel::Severe, true),
        vk::DebugUtilsMessageSeverityFlagsEXT::WARNING => (LogLevel::Warning, true),
        vk::DebugUtilsMessageSeverityFlagsEXT::INFO => (LogLevel::Info, false),
        vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE => (LogLevel::Trace, false),
        _ => { return 0; }
    };
    LOGGER_VK.log(level, message);
    1
}

fn init_vk_debug_utils(inst: &'_ vk::Instance) -> Result<vk::DebugUtilsMessenger<'_>, String> {
    vk::DebugUtilsMessenger::create(inst,
        vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE |
            vk::DebugUtilsMessageSeverityFlagsEXT::INFO |
            vk::DebugUtilsMessageSeverityFlagsEXT::WARNING |
            vk::DebugUtilsMessageSeverityFlagsEXT::ERROR,
        vk::DebugUtilsMessageTypeFlagsEXT::GENERAL |
            vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION,
        debug_callback,
    )
}

fn deinit_vk_debug_utils(debug_messenger: vk::DebugUtilsMessenger) {
    if cfg!(debug_assertions) {
        debug_messenger.destroy();
    }
}

fn activate_vulkan_backend() -> bool {
    WindowManager::instance().set_window_creation_flags(WindowCreationFlags::Vulkan);

    if !vk_is_supported() {
        WindowManager::instance().set_window_creation_flags(WindowCreationFlags::empty());
        return false;
    }

    // create hidden window so we can attach a surface and probe for capabilities
    let Ok(mut window_ref) = WindowManager::instance().create_window("probe_vk") else {
        debug!(LOGGER, "Failed to create window while probing for Vulkan capabilities");
        return false;
    };
    let window = window_ref.value_mut();

    window.update(Duration::from_secs(0));

    let wm_required_exts: Vec<_> = vk_get_required_instance_extensions(window).unwrap()
        .into_iter()
        .map(|ext| CString::new(ext).unwrap())
        .collect();
    let client_name = EngineManager::instance().get_config()
        .get_section::<ClientConfig>().as_ref().unwrap()
        .name.clone();
    let vk_instance = match vk::Instance::load(
        &wm_required_exts.iter().map(|s| s.as_ref()).collect::<Vec<_>>(),
        &client_name,
        &LOGGER,
    ) {
        Ok(inst) => inst,
        Err(err) => {
            debug!(
                LOGGER,
                "Unable to create Vulkan instance: {}",
                err.to_string(),
            );
            WindowManager::instance().set_window_creation_flags(WindowCreationFlags::None);
            window.request_close();
            return false;
        }
    };
    if VK_INSTANCE.set(vk_instance).is_err() {
        panic!("Failed to set global Vulkan instance");
    }
    let vk_instance = VK_INSTANCE.get().unwrap();

    let vk_debug_messenger = if cfg!(debug_assertions) {
        Some(init_vk_debug_utils(vk_instance).unwrap())
    } else {
        None
    };

    /*if (window == nullptr) {
        _deinit_vk_debug_utils(g_vk_instance);
        destroy_vk_instance(g_vk_instance);
        Logger::default_logger().warn("Failed to probe Vulkan capabilities (window creation failed)");
        return false;
    }*/

    let probe_surface_wm = {
        // SAFETY: We just created the Vulkan instance and haven't done anything
        // with it yet so it's guaranteed to be valid, and the wm wrapper only
        // lives to the end of this scope so it cannot outlive `vk_instance`.
        let vk_inst_wm = unsafe {
            argus_wm::VkInstance::from_raw(vk_instance.get_handle())
        };

        match vk_create_surface(window, &vk_inst_wm) {
            Ok(surface) => surface,
            Err(err) => {
                warn!(
                LOGGER,
                "Vulkan does not appear to be supported (failed to create surface: {})",
                err.to_string(),
            );
                WindowManager::instance().set_window_creation_flags(WindowCreationFlags::None);
                if cfg!(debug_assertions) {
                    deinit_vk_debug_utils(vk_debug_messenger.unwrap());
                }
                window.request_close();
                return false;
            },
        }
    };
    // SAFETY: We just created the surface handle, and it is consumed by the
    // wrapper object thus guaranteeing all future interaction will be done
    // through the wrapper.
    let probe_surface = unsafe {
        vk::Surface::from_handle(vk_instance, probe_surface_wm.into_raw())
    };

    let phys_device = match vk::select_physical_device(&vk_instance, &probe_surface, &LOGGER) {
        Ok(dev) => dev,
        Err(err) => {
            info!(
                LOGGER,
                "Vulkan does not appear to be supported (could not get Vulkan device: {})",
                err.to_string()
            );
            if cfg!(debug_assertions) {
                deinit_vk_debug_utils(vk_debug_messenger.unwrap());
            }
            WindowManager::instance().set_window_creation_flags(WindowCreationFlags::None);
            return false;
        }
    };
    let phys_dev_props = phys_device.get_properties();

    info!(
        LOGGER,
        "Selected video device {}",
        phys_dev_props.device_name_as_c_str().map(|s| s.to_string_lossy().to_string())
            .unwrap_or_else(|_| "(unknown)".to_owned()),
    );

    let vk_device_res = vk_instance.create_device(phys_device, &probe_surface, &LOGGER);

    debug!(LOGGER, "Successfully created logical Vulkan device");

    probe_surface.destroy();
    window.request_close();

    let vk_device = match vk_device_res {
        Ok(device) => device,
        Err(err) => {
            info!(
                LOGGER,
                "Vulkan does not appear to be supported (could not get Vulkan device: {})",
                err.to_string(),
            );
            if cfg!(debug_assertions) {
                deinit_vk_debug_utils(vk_debug_messenger.unwrap());
            }
            WindowManager::instance().set_window_creation_flags(WindowCreationFlags::None);
            return false;
        }
    };

    IS_BACKEND_ACTIVE.store(true, atomic::Ordering::Relaxed);

    if VK_DEVICE.set(vk_device).is_err() {
        panic!("Failed to set global Vulkan instance");
    }
    if VK_DEBUG_MESSENGER.set(vk_debug_messenger).is_err() {
        panic!("Failed to set global Vulkan debug messenger");
    }

    true
}

fn window_event_handler(event: &WindowEvent) {
    let window_id = event.window.as_str();
    let Some(mut window) = WindowManager::instance().get_window_mut(window_id) else {
        warn!(LOGGER, "Received window event with unknown window ID!");
        return;
    };

    match event.subtype {
        WindowEventType::Create => {
            // don't create a context if the window was immediately closed
            if !window.is_close_request_pending() {
                let renderer = VulkanRenderer::new(
                    VK_INSTANCE.get().unwrap(),
                    VK_DEVICE.get().unwrap(),
                    &window,
                );
                RENDERERS.with_borrow_mut(|renderers| {
                    renderers.insert(
                        window_id.to_owned(),
                        renderer,
                    )
                });
            }
        }
        WindowEventType::Update => {
            if window.is_ready() {
                RENDERERS.with_borrow_mut(|renderers| {
                    let renderer = renderers.get_mut(window_id)
                        .expect("Failed to get renderer");

                    assert!(renderer.is_initialized());

                    renderer.render(
                        window.value_mut(),
                        event.delta.expect("Window update event did not have delta"),
                    );
                });
            }
        }
        WindowEventType::Resize => {
            if window.is_ready() {
                RENDERERS.with_borrow_mut(|renderers| {
                    let renderer = renderers.get_mut(window_id)
                        .expect("Failed to get renderer");
                    renderer.notify_window_resize(
                        window.value_mut(),
                        event.resolution.expect("Window resize event did not have resolution"),
                    );
                });
            }
        }
        WindowEventType::RequestClose => {
            RENDERERS.with_borrow_mut(|renderers| {
                // This condition fails if the window received a close request
                // immediately, before a context could be created. This is the
                // case when creating a hidden window to probe GL capabilities.
                let Some(renderer) = renderers.remove(window_id) else {
                    return;
                };
                renderer.destroy();
            });
        }
        _ => {}
    }
}

//TODO
/*fn resource_event_handler(event: &ResourceEvent) {
    if (event.subtype != ResourceEventType::Unload) {
        return;
    }

    auto &state = *static_cast<RendererState *>(renderer_state);

    std::string mt = event.prototype.media_type;
    if (mt == RESOURCE_TYPE_SHADER_GLSL_VERT || mt == RESOURCE_TYPE_SHADER_GLSL_FRAG) {
        // no-op for now
    } else if (mt == RESOURCE_TYPE_MATERIAL) {
        deinit_material(state, event.prototype.uid);
    }
}*/

#[register_module(id = "render_vulkan", depends(render, wm))]
pub fn update_lifecycle_render_vulkan(stage: LifecycleStage) {
    match stage {
        LifecycleStage::PreInit => {
            register_render_backend(BACKEND_ID, activate_vulkan_backend);
        }
        LifecycleStage::Init => {
            EngineManager::instance().add_render_init_callback(on_render_init, Ordering::Standard);
        }
        LifecycleStage::PostInit => {
            ResourceManager::instance().add_memory_package(RESOURCES_PACK)
                .expect("Failed to load in-memory resources for render_vulkan");
        }
        LifecycleStage::Deinit => {
            if !IS_BACKEND_ACTIVE.load(atomic::Ordering::Relaxed) {
                return;
            }

            //TODO: destroy Vulkan instance+device
        }
        _ => {}
    }
}

fn on_render_init() {
    if !IS_BACKEND_ACTIVE.load(atomic::Ordering::Relaxed) {
        return;
    }

    ResourceManager::instance().register_loader(
        vec![RESOURCE_TYPE_SHADER_GLSL_FRAG, RESOURCE_TYPE_SHADER_GLSL_VERT],
        ShaderLoader::new(),
    );

    register_event_handler::<WindowEvent>(
        window_event_handler,
        TargetThread::Render,
        Ordering::Standard,
    );
    //TODO
    //register_event_handler::<ResourceEvent>(resource_event_handler, TargetThread::Render);
}

#![feature(used_with_arg)]

pub mod common;
pub mod constants;
pub mod twod;
pub mod util;

mod resources;
mod loader;

use argus_logging::{debug, info, warn};
use argus_core::*;
use argus_resman::ResourceManager;
use argus_wm::WindowManager;
use crate::common::{activate_backend, RenderCanvas};
use crate::constants::*;
use crate::loader::material_loader::MaterialLoader;
use crate::loader::texture_loader::TextureLoader;
use crate::resources::RESOURCES_PACK;

argus_logging::crate_logger!(LOGGER, "argus/render");

const RENDER_BACKEND_MODULE_PREFIX: &str = "render_";

#[register_module(id = "render", depends(core, resman, scripting, wm))]
pub fn update_lifecycle_render(stage: LifecycleStage) {
    match stage {
        LifecycleStage::Load => {
            load_backend_modules();
        }
        LifecycleStage::PreInit => {}
        LifecycleStage::Init => {
            WindowManager::instance().set_canvas_ctor(
                Box::new(|window| Box::new(RenderCanvas::new(window)))
            );

            EngineManager::instance().add_render_init_callback(
                || activate_backend(),
                Ordering::Early,
            );

            ResourceManager::instance()
                .register_loader(vec![RESOURCE_TYPE_MATERIAL], MaterialLoader {});
            ResourceManager::instance()
                .register_loader(vec![RESOURCE_TYPE_TEXTURE_PNG], TextureLoader {});
        }
        LifecycleStage::PostInit => {
            ResourceManager::instance().add_memory_package(RESOURCES_PACK)
                .expect("Failed to load in-memory resources for render");
        }
        LifecycleStage::Running => {}
        LifecycleStage::PreDeinit => {}
        LifecycleStage::Deinit => {}
        LifecycleStage::PostDeinit => {}
    }
}

fn load_backend_modules() {
    debug!(LOGGER, "Loading graphics backend modules");
    let mut count = 0;
    for module_id in get_present_dynamic_modules() {
        if module_id.rfind(RENDER_BACKEND_MODULE_PREFIX) == Some(0) {
            //TODO: fail gracefully
            if !enable_dynamic_module(&module_id) {
                warn!(LOGGER, "Failed to load render backend \"{}\"", module_id);
            }
            count += 1;
        }
    }
    for module_id in get_present_static_modules() {
        if module_id.rfind(RENDER_BACKEND_MODULE_PREFIX) == Some(0) {
            //TODO: this is currently non-functional
            EngineManager::instance().get_config_mut().load_modules.push(module_id);
            count += 1;
        }
    }
    info!(LOGGER, "Loaded {} graphics backend modules", count);
}

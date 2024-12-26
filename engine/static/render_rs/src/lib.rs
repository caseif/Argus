pub mod common;
pub mod constants;
pub mod twod;
pub mod util;

mod resources;
mod loader;

use std::ffi;
use num_enum::UnsafeFromPrimitive;
use core_rustabi::argus::core::{add_load_module, enable_dynamic_module, get_present_dynamic_modules, get_present_static_modules, LifecycleStage};
use resman_rustabi::argus::resman::ResourceManager;
use uuid::Uuid;
use wm_rustabi::argus::wm::Window;
use crate::common::activate_backend;
use crate::constants::{RESOURCE_TYPE_MATERIAL, RESOURCE_TYPE_TEXTURE_PNG};
use crate::loader::material_loader::MaterialLoader;
use crate::loader::texture_loader::TextureLoader;
use crate::resources::RESOURCES_PACK;
use crate::twod::get_render_context_2d;


const RENDER_BACKEND_MODULE_PREFIX: &str = "render_";

#[no_mangle]
pub unsafe extern "C" fn update_lifecycle_render_rs(
    stage_ffi: core_rustabi::core_cabi::LifecycleStage
) {
    let stage = unsafe { LifecycleStage::unchecked_transmute_from(stage_ffi) };

    match stage {
        LifecycleStage::Load => {
            load_backend_modules();
        }
        LifecycleStage::PreInit => {}
        LifecycleStage::Init => {
            unsafe extern "C" fn canvas_ctor(window_handle: *mut ffi::c_void) -> *mut ffi::c_void {
                let ptr = Box::into_raw(Box::new(
                    get_render_context_2d().create_canvas(Window::of(window_handle)).get_id()
                )).cast();
                ptr
            }
            unsafe extern "C" fn canvas_dtor(canvas_ptr: *mut ffi::c_void) {
                let canvas_id = canvas_ptr.cast::<Uuid>().as_ref().unwrap();
                get_render_context_2d().remove_canvas(canvas_id);
            }

            Window::set_canvas_ctor_and_dtor(Some(canvas_ctor), Some(canvas_dtor));

            activate_backend();

            ResourceManager::get_instance()
                .register_loader(vec![RESOURCE_TYPE_MATERIAL], MaterialLoader {});
            ResourceManager::get_instance()
                .register_loader(vec![RESOURCE_TYPE_TEXTURE_PNG], TextureLoader {});
        }
        LifecycleStage::PostInit => {
            ResourceManager::get_instance().add_memory_package(RESOURCES_PACK);
        }
        LifecycleStage::Running => {}
        LifecycleStage::PreDeinit => {}
        LifecycleStage::Deinit => {}
        LifecycleStage::PostDeinit => {}
    }
}

fn load_backend_modules() {
    println!("Loading graphics backend modules");
    let mut count = 0;
    for module_id in get_present_dynamic_modules() {
        if module_id.rfind(RENDER_BACKEND_MODULE_PREFIX) == Some(0) {
            //TODO: fail gracefully
            if !enable_dynamic_module(&module_id) {
                println!("Failed to load render backend \"{}\"", module_id);
            }
            count += 1;
        }
    }
    for module_id in get_present_static_modules() {
        if module_id.rfind(RENDER_BACKEND_MODULE_PREFIX) == Some(0) {
            //TODO: this is currently non-functional
            add_load_module(&module_id);
            count += 1;
        }
    }
    println!("Loaded {} graphics backend modules", count);
}

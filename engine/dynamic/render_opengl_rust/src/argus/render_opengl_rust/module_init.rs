/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

use std::collections::HashMap;
use std::sync::{Mutex};
use std::time::Duration;

use lazy_static::lazy_static;
use num_enum::UnsafeFromPrimitive;

use core_rustabi::argus::core::*;
use render_rustabi::argus::render::*;
use resman_rustabi::argus::resman::{ResourceEvent, ResourceEventType, ResourceManager};
use wm_rustabi::argus::wm::*;

use crate::aglet::{AgletError, agletLoadCapabilities};
use crate::argus::render_opengl_rust::gl_renderer::GlRenderer;
use crate::argus::render_opengl_rust::resources::RESOURCES_PACK;

const MODULE_ID: &'static str = "render_opengl_rust";
const BACKEND_ID: &'static str = "opengl_rust";

lazy_static! {
    static ref g_is_backend_active: Mutex<bool> = Mutex::new(false);
    static ref g_renderer_map: Mutex<HashMap<String, GlRenderer>>
        = Mutex::new(HashMap::new());
}

fn test_opengl_support() -> Result<(), ()> {
    let mut window = Window::create("", None);
    window.update(Duration::from_secs(0));
    let gl_context_opt = gl_create_context(&mut window, 3, 3, GLContextFlags::ProfileCore);
    if gl_context_opt.is_none() {
        //TODO: use proper logger
        eprintln!("Failed to create GL context");
        return Err(());
    }

    let gl_context = gl_context_opt.unwrap();

    if let Err(rc) = gl_make_context_current(&mut window, &gl_context) {
        //TODO: use proper logger
        eprintln!("Failed to make GL context current ({rc})");
        return Err(());
    }

    if let Err(e) = agletLoadCapabilities(gl_load_proc) {
        let err_msg = match e {
            AgletError::Unspecified => "Aglet failed to load OpenGL bindings (unspecified error)",
            AgletError::ProcLoad => "Aglet failed to load required OpenGL procs",
            AgletError::GlError => "Aglet failed to load OpenGL bindings (OpenGL error)",
            AgletError::MinVersion => "Argus requires support for OpenGL 3.3 or higher",
            AgletError::MissingExtension => "Required OpenGL extensions are not available",
        };
        //TODO: use proper logger
        eprintln!("{}", err_msg);
        return Err(())
    }

    window.request_close();

    return Ok(());
}

extern "C" fn activate_opengl_backend() -> bool {
    set_window_create_flags(WindowCreateFlags::OpenGL);

    if gl_load_library() != 0 {
        //Logger::default_logger().warn("Failed to load OpenGL library");
        set_window_create_flags(WindowCreateFlags::None);
        return false;
    }

    if test_opengl_support().is_err() {
        gl_unload_library();
        set_window_create_flags(WindowCreateFlags::None);
        return false;
    }

    *g_is_backend_active.lock().unwrap() = true;
    return true;
}

fn window_event_handler(event: &WindowEvent) {
    let window = event.get_window();

    match event.get_subtype() {
        WindowEventType::Create => {
            // don't create a context if the window was immediately closed
            if !window.is_close_request_pending() {
                let renderer = GlRenderer::new(window.clone());
                g_renderer_map.lock().unwrap().insert(window.get_id(), renderer);
            }
        }
        WindowEventType::Update => {
            if window.is_ready() {
                let mut renderer_map_guard = g_renderer_map.lock().unwrap();
                let renderer = renderer_map_guard.get_mut(&window.get_id())
                    .expect("Failed to get renderer");
                renderer.render(event.get_delta());
            }
        }
        WindowEventType::Resize => {
            if window.is_ready() {
                let mut renderer_map_guard = g_renderer_map.lock().unwrap();
                let renderer = renderer_map_guard.get_mut(&window.get_id())
                    .expect("Failed to get renderer");
                renderer.notify_window_resize(event.get_resolution());
           }
        }
        WindowEventType::RequestClose => {
            let renderer_map_guard = g_renderer_map.lock().unwrap();
            // This condition fails if the window received a close request
            // immediately, before a context could be created. This is the
            // case when creating a hidden window to probe GL capabilities.
            if renderer_map_guard.contains_key(&window.get_id()) {
                //TODO: delete renderer
            }
        }
        _ => {}
    }
}

fn resource_event_handler(event: &ResourceEvent) {
    if event.get_subtype() != ResourceEventType::Unload {
        return;
    }

    for (_, renderer) in g_renderer_map.lock().unwrap().iter() {
        let mt = event.get_prototype().media_type;
        if mt == RESOURCE_TYPE_SHADER_GLSL_VERT || mt == RESOURCE_TYPE_SHADER_GLSL_FRAG {
            //TODO: remove shader from state
        } else if mt == RESOURCE_TYPE_MATERIAL {
            //TODO: deinit material
        }
    }
}

#[no_mangle]
pub extern "C" fn update_lifecycle_render_opengl_rust(
    stage_c: core_rustabi::core_cabi::LifecycleStage
) {
    let stage = unsafe { LifecycleStage::unchecked_transmute_from(stage_c) };

    match stage {
        LifecycleStage::PreInit => {
            println!("render_opengl_rust got PreInit event");
            register_render_backend(BACKEND_ID, Some(activate_opengl_backend));
        }
        LifecycleStage::Init => {
            println!("render_opengl_rust got Init event");
            if !*g_is_backend_active.lock().unwrap() {
                return;
            }

            //TODO: register shader loader

            register_event_handler(
                &window_event_handler,
                TargetThread::Render,
                Ordering::Standard
            );
            register_event_handler(
                &resource_event_handler,
                TargetThread::Render,
                Ordering::Standard
            );
        }
        LifecycleStage::PostInit => {
            if !*g_is_backend_active.lock().unwrap() {
                return;
            }

            ResourceManager::get_instance().add_memory_package(RESOURCES_PACK);
        }
        LifecycleStage::PostDeinit => {
            gl_unload_library();
        }
        _ => {}
    }
}

#[no_mangle]
pub extern "C" fn register_plugin() {
    register_dynamic_module(
        MODULE_ID,
        update_lifecycle_render_opengl_rust,
        vec![],
    );
}

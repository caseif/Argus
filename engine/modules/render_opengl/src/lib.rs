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

#![feature(used_with_arg)]

pub(crate) mod aglet;

pub(crate) mod loader;
pub(crate) mod state;
pub(crate) mod twod;
pub(crate) mod util;
pub(crate) mod bucket_proc;
pub(crate) mod gl_renderer;
pub(crate) mod materials;
pub(crate) mod resources;
pub(crate) mod shaders;
pub(crate) mod textures;
mod compositing;

use std::cell::RefCell;
use std::collections::HashMap;
use std::ops::DerefMut;
use std::time::Duration;
use argus_logging::{crate_logger, warn};
use argus_core::{register_event_handler, register_module, EngineManager, LifecycleStage, Ordering, TargetThread};
use argus_render::common::register_render_backend;
use argus_render::constants::{RESOURCE_TYPE_MATERIAL, RESOURCE_TYPE_SHADER_GLSL_FRAG, RESOURCE_TYPE_SHADER_GLSL_VERT};
use argus_resman::{ResourceEvent, ResourceEventType, ResourceManager};
use argus_wm::*;
use crate::aglet::{AgletError, agletLoadCapabilities};
use crate::gl_renderer::GlRenderer;
use crate::loader::ShaderLoader;
use crate::resources::RESOURCES_PACK;

const BACKEND_ID: &str = "opengl";

crate_logger!(LOGGER, "argus/render_opengl");
crate_logger!(GL_LOGGER, "opengl");

thread_local! {
    static IS_BACKEND_ACTIVE: RefCell<bool> = RefCell::new(false);
    static RENDERERS: RefCell<HashMap<String, GlRenderer>>
        = RefCell::new(HashMap::new());
}

fn is_backend_active() -> bool {
    IS_BACKEND_ACTIVE.with_borrow(|active| *active)
}

fn set_backend_active(active: bool) {
    IS_BACKEND_ACTIVE.set(active);
}

fn test_opengl_support() -> Result<(), ()> {
    let mut window = WindowManager::instance().create_window("gl_probe", None)
        .expect("Failed to create window for OpenGL probe");
    window.update(Duration::from_secs(0));
    let gl_mgr = WindowManager::instance().get_gl_manager().expect("Failed to get OpenGL manager");
    let gl_context =
        match gl_mgr.create_context(&window, 3, 3, GlContextFlags::ProfileCore.into()) {
            Ok(ctx) => ctx,
            Err(e) => {
                warn!(LOGGER, "Failed to create GL context: {e}");
                return Err(());
            }
        };

    if let Err(rc) = gl_context.make_current(&window) {
        warn!(LOGGER, "Failed to make GL context current ({rc})");
        return Err(());
    }

    if let Err(e) = agletLoadCapabilities(GlManager::load_proc_ffi) {
        let err_msg = match e {
            AgletError::Unspecified => "Aglet failed to load OpenGL bindings (unspecified error)",
            AgletError::ProcLoad => "Aglet failed to load required OpenGL procs",
            AgletError::GlError => "Aglet failed to load OpenGL bindings (OpenGL error)",
            AgletError::MinVersion => "Argus requires support for OpenGL 3.3 or higher",
            AgletError::MissingExtension => "Required OpenGL extensions are not available",
        };
        warn!(LOGGER, "{err_msg}");
        return Err(());
    }

    window.request_close();

    Ok(())
}

fn activate_opengl_backend() -> bool {
    let mgr = WindowManager::instance();

    let std_flags = WindowCreationFlags::OpenGL;
    let transient_flags = std_flags | WindowCreationFlags::Transient;

    mgr.set_window_creation_flags(transient_flags);

    let gl_mgr = mgr.get_gl_manager().expect("Failed to get OpenGL manager");
    
    if let Err(err) = gl_mgr.load_library() {
        warn!(LOGGER, "Failed to load OpenGL library: {}", err);
        mgr.set_window_creation_flags(WindowCreationFlags::None);
        return false;
    }

    if test_opengl_support().is_err() {
        gl_mgr.unload_library();
        mgr.set_window_creation_flags(WindowCreationFlags::None);
        return false;
    }

    mgr.set_window_creation_flags(std_flags);

    set_backend_active(true);
    true
}

fn window_event_handler(event: &WindowEvent) {
    let window_id = &event.window;
    let Some(mut window) = WindowManager::instance().get_window_mut(window_id) else {
        warn!(LOGGER, "Received window event with unknown window ID!");
        return;
    };

    match event.subtype {
        WindowEventType::Create => {
            // don't create a context if the window was immediately closed
            if !window.is_close_request_pending() {
                let renderer = GlRenderer::new(&mut *window);
                RENDERERS.with_borrow_mut(|renderers| {
                    renderers.insert(window_id.clone(), renderer)
                });
            }
        }
        WindowEventType::Update => {
            if window.is_ready() {
                RENDERERS.with_borrow_mut(|renderers| {
                    let renderer = renderers.get_mut(window_id)
                        .expect("Failed to get renderer");
                    renderer.render(
                        window.deref_mut(),
                        event.delta.expect("Window update event did not have delta")
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
                        window.deref_mut(),
                        &event.resolution.expect("Window resize event did not have resolution")
                    );
                });
            }
        }
        WindowEventType::RequestClose => {
            RENDERERS.with_borrow_mut(|renderers| {
                // This condition fails if the window received a close request
                // immediately, before a context could be created. This is the
                // case when creating a hidden window to probe GL capabilities.
                if renderers.contains_key(window_id) {
                    //TODO: delete renderer
                }
            });
        }
        _ => {}
    }
}

fn resource_event_handler(event: &ResourceEvent) {
    if event.get_subtype() != ResourceEventType::Unload {
        return;
    }

    RENDERERS.with_borrow_mut(|renderers| {
        #[allow(unused)]
        for (_, renderer) in &mut renderers.iter() {
            let mt = &event.get_prototype().media_type;
            if mt == RESOURCE_TYPE_SHADER_GLSL_VERT || mt == RESOURCE_TYPE_SHADER_GLSL_FRAG {
                //TODO: remove shader from state
            } else if mt == RESOURCE_TYPE_MATERIAL {
                //TODO: deinit material
            }
        }
    });
}

#[register_module(id = "render_opengl", depends(core, render, resman, scripting, wm))]
pub fn update_lifecycle_render_opengl(stage: LifecycleStage) {
    match stage {
        LifecycleStage::PreInit => {
            register_render_backend(BACKEND_ID, activate_opengl_backend);
        }
        LifecycleStage::Init => {
            EngineManager::instance().add_render_init_callback(on_render_init, Ordering::Standard);
        }
        LifecycleStage::PostInit => {
        }
        LifecycleStage::Deinit => {
            RENDERERS.with_borrow_mut(|renderers| renderers.clear() );
        }
        LifecycleStage::PostDeinit => {
            //WindowManager::instance().get_gl_manager().unwrap().unload_library();
        }
        _ => {}
    }
}

fn on_render_init() {
    if !is_backend_active() {
        return;
    }

    ResourceManager::instance().register_loader(
        vec![
            RESOURCE_TYPE_SHADER_GLSL_VERT,
            RESOURCE_TYPE_SHADER_GLSL_FRAG,
        ],
        ShaderLoader::new(),
    );

    register_event_handler(
        Box::new(window_event_handler),
        TargetThread::Render,
        Ordering::Standard
    );
    register_event_handler(
        Box::new(resource_event_handler),
        TargetThread::Render,
        Ordering::Standard
    );

    ResourceManager::instance().add_memory_package(RESOURCES_PACK)
        .expect("Failed to load in-memory resources for render_opengl");
}

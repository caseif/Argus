use std::collections::HashMap;
use std::sync::Mutex;
use argus_logging::{debug, info, warn};
use core_rustabi::argus::core::get_preferred_render_backends;
use lazy_static::lazy_static;
use crate::LOGGER;

type ActivateRenderBackendFn = fn() -> bool;

#[cfg(any(
    target_os = "linux",
    target_os = "android"
))]
const DEFAULT_BACKENDS: &[&str] = &["opengl", "opengl_es"];
#[cfg(
    target_os = "windows"
)]
const DEFAULT_BACKENDS: &[&str] = &["opengl", "opengl_es"];
#[cfg(not(any(
    target_os = "linux",
    target_os = "android",
    target_os = "windows"
)))]
const DEFAULT_BACKENDS: &[&str] = &["opengl"];

lazy_static! {
    static ref BACKEND_REGISTRY: Mutex<HashMap<String, ActivateRenderBackendFn>> =
        Default::default();
    static ref ACTIVE_BACKEND: Mutex<Option<String>> = Default::default();
}

pub fn register_render_backend(id: impl AsRef<str>, activate_fn: ActivateRenderBackendFn) {
    if BACKEND_REGISTRY.lock().unwrap().insert(id.as_ref().to_string(), activate_fn).is_some() {
        panic!("Render backend is already registered for provided ID")
    }
    info!(LOGGER, "Successfully registered render backend with ID {}", id.as_ref());
}

#[allow(unused)]
pub(crate) fn get_render_backend_activate_fn(backend_id: impl AsRef<str>)
    -> Option<ActivateRenderBackendFn> {
    BACKEND_REGISTRY.lock().unwrap().get(backend_id.as_ref()).cloned()
}

pub fn unregister_backend_activate_fns() {
    BACKEND_REGISTRY.lock().unwrap().clear();
}

#[must_use]
pub fn get_active_render_backend() -> Option<String> {
    ACTIVE_BACKEND.lock().unwrap().clone()
}

pub fn set_active_render_backend(backend: impl Into<String>) {
    *ACTIVE_BACKEND.lock().unwrap() = Some(backend.into())
}

fn try_backends(
    backends: impl IntoIterator<Item = impl Into<String>>,
    attempted_backends: &mut Vec<String>
) -> bool {
    for backend in backends.into_iter() {
        let backend_str = backend.into();

        if attempted_backends.contains(&backend_str) {
            debug!(LOGGER, "Skipping graphics \"{}\" because we already tried it", backend_str);
            continue;
        }

        let Some(activate_fn) = get_render_backend_activate_fn(&backend_str) else {
            info!(LOGGER, "Skipping unknown graphics backend \"{}\"", backend_str);
            attempted_backends.push(backend_str.clone());
            continue;
        };

        let activate_res = activate_fn();
        if !activate_res {
            info!(LOGGER, "Unable to select graphics backend \"{}\"", backend_str);
            attempted_backends.push(backend_str.clone());
            continue;
        }

        info!(LOGGER, "Successfully activated graphics backend \"{}\"", backend_str);

        set_active_render_backend(backend_str);

        return true;
    }

    false
}

pub(crate) fn activate_backend() {
    let backends = get_preferred_render_backends();

    let mut attempted_backends = Vec::new();

    if try_backends(&backends, &mut attempted_backends) {
        return;
    }

    warn!(
        LOGGER,
        "Failed to select graphics backend from preference list, falling back to platform default"
    );

    if try_backends(DEFAULT_BACKENDS.to_owned(), &mut attempted_backends) {
        return;
    }

    panic!("Failed to select graphics backend");
}

use std::sync::{Arc, LazyLock, Mutex};
use argus_util::math::{Vector2i, Vector2u};

//TODO: temporary hack while rewriting in Rust
static DS_CONFIG: LazyLock<Arc<Mutex<DownstreamConfig>>> =
    LazyLock::new(|| Arc::new(Mutex::new(DownstreamConfig::default())));

#[derive(Debug, Default)]
struct DownstreamConfig {
    script_params: ScriptingParameters,
    win_params: InitialWindowParameters,
    bindings_res_id: String,
    save_user_bindings: bool,
}

#[derive(Clone, Debug, Default)]
pub struct ScriptingParameters {
    pub main: Option<String>,
}

#[derive(Clone, Debug, Default)]
pub struct InitialWindowParameters {
    pub id: Option<String>,
    pub title: Option<String>,
    pub mode: Option<String>,
    pub vsync: Option<bool>,
    pub mouse_visible: Option<bool>,
    pub mouse_captured: Option<bool>,
    pub mouse_raw_input: Option<bool>,
    pub position: Option<Vector2i>,
    pub dimensions: Option<Vector2u>,
}

pub fn get_scripting_parameters() -> ScriptingParameters {
    DS_CONFIG.lock().unwrap().script_params.clone()
}

pub fn set_scripting_parameters(params: ScriptingParameters) {
    DS_CONFIG.lock().unwrap().script_params = params;
}

pub fn get_initial_window_parameters() -> InitialWindowParameters {
    DS_CONFIG.lock().unwrap().win_params.clone()
}

pub fn set_initial_window_parameters(params: InitialWindowParameters) {
    DS_CONFIG.lock().unwrap().win_params = params;
}

pub fn get_default_bindings_resource_id() -> String {
    DS_CONFIG.lock().unwrap().bindings_res_id.clone()
}

pub fn set_default_bindings_resource_id(resource_id: impl Into<String>) {
    DS_CONFIG.lock().unwrap().bindings_res_id = resource_id.into();
}

pub fn get_save_user_bindings() -> bool {
    DS_CONFIG.lock().unwrap().save_user_bindings
}

pub fn set_save_user_bindings(save: bool) {
    DS_CONFIG.lock().unwrap().save_user_bindings = save;
}

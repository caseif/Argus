use serde::Deserialize;

#[derive(Clone, Debug, Deserialize)]
pub(crate) struct BindingsConfig {
    #[serde(default)]
    default_bindings_resource: Option<String>,
    #[serde(default)]
    save_user_bindings: Option<bool>,
}
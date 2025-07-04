use serde::Deserialize;

#[allow(unused)]
#[derive(Clone, Debug, Deserialize)]
pub(crate) struct BindingsConfig {
    #[serde(default)]
    pub(crate) default_bindings_resource: Option<String>,
    #[serde(default)]
    pub(crate) save_user_bindings: Option<bool>,
}
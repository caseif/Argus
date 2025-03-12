use serde::Deserialize;

#[derive(Clone, Debug, Deserialize)]
pub(crate) struct ScriptingConfig {
    #[serde(default)]
    pub main: Option<String>,
}
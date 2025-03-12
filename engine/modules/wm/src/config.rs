use argus_util::math::{Vector2i, Vector2u};
use serde::Deserialize;

#[derive(Clone, Debug, Deserialize)]
pub(crate) struct WindowConfig {
    #[serde(default)]
    pub id: Option<String>,
    #[serde(default)]
    pub title: Option<String>,
    #[serde(default)]
    pub mode: Option<String>,
    #[serde(default)]
    pub vsync: Option<bool>,
    #[serde(default)]
    pub mouse_visible: Option<bool>,
    #[serde(default)]
    pub mouse_captured: Option<bool>,
    #[serde(default)]
    pub mouse_raw_input: Option<bool>,
    #[serde(default)]
    pub position: Option<Vector2i>,
    pub dimensions: Vector2u,
}
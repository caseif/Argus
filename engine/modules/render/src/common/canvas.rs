use std::any::Any;
use std::collections::HashMap;
use uuid::Uuid;
use argus_util::error::ArgumentError;
use argus_util::math::Vector2f;
use argus_wm::{Canvas, Window};
use crate::common::{AttachedViewport, Viewport, ViewportCoordinateSpaceMode};
use crate::twod::AttachedViewport2d;

#[derive(Debug)]
pub struct RenderCanvas {
    id: Uuid,
    window_id: String,
    viewports_2d: HashMap<u32, AttachedViewport2d>,
}

impl Canvas for RenderCanvas {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

impl RenderCanvas {
    #[must_use]
    pub(crate) fn new(window: &Window) -> Self {
        //TODO: validate window
        Self {
            id: Uuid::new_v4(),
            window_id: window.get_id().to_string(),
            viewports_2d: HashMap::new()
        }
    }

    #[must_use]
    pub fn get_id(&self) -> Uuid {
        self.id
    }

    #[must_use]
    pub fn get_window_id(&self) -> &str {
        self.window_id.as_str()
    }

    #[must_use]
    pub fn get_viewports_2d(&self) -> Vec<&AttachedViewport2d> {
        self.viewports_2d.values().collect()
    }

    #[must_use]
    pub fn get_viewports_2d_mut(&mut self) -> Vec<&mut AttachedViewport2d> {
        self.viewports_2d.values_mut().collect()
    }

    #[must_use]
    pub fn get_viewport(&self, id: u32) -> Option<&AttachedViewport2d> {
        self.viewports_2d.get(&id)
    }

    pub fn add_viewport_2d(
        &mut self,
        scene_id: impl AsRef<str>,
        camera_id: impl AsRef<str>,
        viewport: Viewport,
        z_index: u32
    ) -> Result<&AttachedViewport2d, ArgumentError> {
        //TODO: validate camera arg
        let viewport =
            AttachedViewport2d::new(scene_id.as_ref(), camera_id.as_ref(), viewport, z_index);
        let id = viewport.get_id();
        self.viewports_2d.insert(id, viewport);
        Ok(self.viewports_2d.get(&id).unwrap())
    }

    pub fn add_default_viewport_2d(
        &mut self,
        scene_id: impl AsRef<str>,
        camera_id: impl AsRef<str>,
        z_index: u32
    ) -> Result<&AttachedViewport2d, ArgumentError> {
        let viewport = Viewport {
            top: 0.0,
            left: 0.0,
            bottom: 1.0,
            right: 1.0,
            scaling: Vector2f::new(1.0, 1.0),
            mode: ViewportCoordinateSpaceMode::Individual,
        };
        self.add_viewport_2d(scene_id.as_ref(), camera_id.as_ref(), viewport, z_index)
    }

    pub fn remove_viewport_2d(&mut self, id: u32) -> bool {
        self.viewports_2d.remove(&id).is_some()
    }
}

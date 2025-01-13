use std::collections::HashMap;
use lowlevel_rs::{ArgumentError, arg_error, validate_arg};
use lowlevel_rustabi::argus::lowlevel::Vector2f;
use wm_rustabi::argus::wm::Window;
use uuid::Uuid;
use crate::common::{Viewport, ViewportCoordinateSpaceMode};
use crate::twod::AttachedViewport2d;

#[derive(Debug)]
pub struct Canvas {
    id: Uuid,
    window: Window,
    viewports_2d: HashMap<String, AttachedViewport2d>,
}

impl Canvas {
    #[must_use]
    pub(crate) fn new(window: Window) -> Self {
        //TODO: validate window
        Self { id: Uuid::new_v4(), window, viewports_2d: HashMap::new() }
    }

    #[must_use]
    pub fn get_id(&self) -> Uuid {
        self.id
    }

    #[must_use]
    pub fn get_window(&self) -> Window {
        self.window
    }

    #[must_use]
    pub fn get_viewports_2d(&self) -> Vec<&AttachedViewport2d> {
        self.viewports_2d.values().collect()
    }

    #[must_use]
    pub fn get_viewport(&self, id: impl AsRef<str>) -> Option<&AttachedViewport2d> {
        self.viewports_2d.get(id.as_ref())
    }

    pub fn add_viewport_2d(
        &mut self,
        id: impl AsRef<str>,
        scene_id: impl AsRef<str>,
        camera_id: impl AsRef<str>,
        viewport: Viewport,
        z_index: u32
    ) -> Result<&AttachedViewport2d, ArgumentError> {
        validate_arg!(
            id,
            !self.viewports_2d.contains_key(id.as_ref()),
            "Viewport with same ID is already attached"
        );
        //TODO: validate camera arg

        self.viewports_2d.insert(
            id.as_ref().to_string(),
            AttachedViewport2d::new(scene_id.as_ref(), camera_id.as_ref(), viewport, z_index)
        );
        Ok(self.viewports_2d.get(id.as_ref()).unwrap())
    }

    pub fn add_default_viewport_2d(
        &mut self,
        id: impl AsRef<str>,
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
        self.add_viewport_2d(id, scene_id.as_ref(), camera_id.as_ref(), viewport, z_index)
    }

    pub fn remove_viewport_2d(&mut self, id: impl AsRef<str>) -> bool {
        self.viewports_2d.remove(id.as_ref()).is_some()
    }
}

use std::any::Any;
use std::collections::{HashMap, HashSet};
use dashmap::mapref::one::{Ref, RefMut};
use uuid::Uuid;
use argus_util::error::ArgumentError;
use argus_util::math::Vector2f;
use argus_wm::{Canvas, Window};
use crate::common::{AttachedViewport, Viewport, ViewportCoordinateSpaceMode};
use crate::twod::{get_render_context_2d, AttachedViewport2d};

#[derive(Debug)]
pub struct RenderCanvas {
    id: Uuid,
    window_id: String,
    // key is ID, value is z index
    viewports_2d: HashMap<u32, u32>,
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
            viewports_2d: HashMap::new(),
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
    pub fn get_viewports_2d(&self) -> Vec<u32> {
        let mut vps = self.viewports_2d.iter().collect::<Vec<_>>();
        vps.sort_by_key(|(_, z_index)| *z_index);
        vps.into_iter().map(|(id, _)| *id).collect()
    }

    #[must_use]
    pub fn get_viewport_2d(&self, id: u32) -> Option<Ref<u32, AttachedViewport2d>> {
        if !self.viewports_2d.contains_key(&id) {
            return None;
        }

        Some(
            get_render_context_2d().get_viewport(id)
                .expect("Viewport was missing from render context!")
        )
    }

    #[must_use]
    pub fn get_viewport_2d_mut(&self, id: u32) -> Option<RefMut<u32, AttachedViewport2d>> {
        if !self.viewports_2d.contains_key(&id) {
            return None;
        }

        Some(
            get_render_context_2d().get_viewport_mut(id)
                .expect("Viewport was missing from render context!")
        )
    }

    pub fn add_viewport_2d(
        &mut self,
        scene_id: impl AsRef<str>,
        camera_id: impl AsRef<str>,
        viewport: Viewport,
        z_index: u32
    ) -> Result<RefMut<u32, AttachedViewport2d>, ArgumentError> {
        //TODO: validate scene arg
        //TODO: validate camera arg
        let vp = get_render_context_2d().create_viewport(scene_id, camera_id, viewport, z_index);
        self.viewports_2d.insert(vp.get_id(), vp.get_z_index());
        Ok(vp)
    }

    pub fn add_default_viewport_2d(
        &mut self,
        scene_id: impl AsRef<str>,
        camera_id: impl AsRef<str>,
        z_index: u32
    ) -> Result<RefMut<u32, AttachedViewport2d>, ArgumentError> {
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
        get_render_context_2d().remove_viewport(id);
        self.viewports_2d.remove(&id).is_some()
    }
}

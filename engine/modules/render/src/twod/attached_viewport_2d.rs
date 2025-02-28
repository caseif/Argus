use crate::common::{get_next_viewport_id, AttachedViewport, Viewport};

#[derive(Debug)]
pub struct AttachedViewport2d {
    id: u32,
    scene_id: String,
    camera_id: String,
    viewport: Viewport,
    z_index: u32,
    postfx_shaders: Vec<String>,
}

impl AttachedViewport for AttachedViewport2d {
    fn get_id(&self) -> u32 {
        self.id
    }

    fn get_viewport(&self) -> &Viewport {
        &self.viewport
    }

    fn get_z_index(&self) -> u32 {
        self.z_index
    }

    fn get_postprocessing_shaders(&self) -> &Vec<String> {
        &self.postfx_shaders
    }

    fn add_postprocessing_shader(&mut self, shader_uid: String) {
        self.postfx_shaders.push(shader_uid);
    }

    fn remove_postprocessing_shader(&mut self, shader_uid: String) {
        self.postfx_shaders.retain(|x| x != &shader_uid);
    }
}

impl AttachedViewport2d {
    #[must_use]
    pub(crate) fn new(
        scene_id: impl Into<String>,
        camera_id: impl Into<String>,
        viewport: Viewport,
        z_index: u32
    ) -> Self {
        Self {
            id: get_next_viewport_id(),
            scene_id: scene_id.into(),
            camera_id: camera_id.into(),
            viewport,
            z_index,
            postfx_shaders: vec![],
        }
    }

    pub fn get_scene_id(&self) -> &str {
        self.scene_id.as_str()
    }

    pub fn get_camera_id(&self) -> &str {
        self.camera_id.as_str()
    }
}

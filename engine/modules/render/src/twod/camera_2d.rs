use argus_util::versioned::Versioned;
use crate::common::Transform2d;

pub struct Camera2d {
    id: String,
    scene_id: String,
    transform: Versioned<Transform2d>,
}

impl Camera2d {
    #[must_use]
    pub fn new(id: String, scene_id: impl Into<String>, transform: Transform2d) -> Self {
        Self { id, scene_id: scene_id.into(), transform: Versioned::new(transform) }
    }

    #[must_use]
    pub fn get_id(&self) -> &str {
        self.id.as_str()
    }

    #[must_use]
    pub fn get_scene_id(&self) -> &str {
        self.scene_id.as_str()
    }

    #[must_use]
    pub fn get_transform(&self) -> &Versioned<Transform2d> {
        &self.transform
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        self.transform.set(transform)
    }
}

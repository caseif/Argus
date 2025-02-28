#[derive(Clone, Debug)]
pub struct Material {
    pub texture_uid: String,
    pub shader_uids: Vec<String>,
}

impl Material {
    #[must_use]
    pub fn new(texture_uid: impl Into<String>, shader_uids: Vec<String>) -> Self {
        Self {
            texture_uid: texture_uid.into(),
            shader_uids,
        }
    }

    #[must_use]
    pub fn get_texture_uid(&self) -> &String {
        &self.texture_uid
    }

    #[must_use]
    pub fn get_shader_uids(&self) -> &Vec<String> {
        &self.shader_uids
    }
}

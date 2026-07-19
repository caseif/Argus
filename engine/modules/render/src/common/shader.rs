#[derive(Clone, Debug)]
pub struct Shader<W> {
    uid: String,
    stage: ShaderStage,
    src: Vec<W>,
}

impl<W> Shader<W> {
    #[must_use]
    pub fn new(
        uid: impl Into<String>,
        stage: ShaderStage,
        src: impl Into<Vec<W>>,
    ) -> Self {
        Self { uid: uid.into(), stage, src: src.into() }
    }

    #[must_use]
    pub fn get_uid(&self) -> &str {
        self.uid.as_str()
    }

    #[must_use]
    pub fn get_stage(&self) -> ShaderStage {
        self.stage
    }

    #[must_use]
    pub fn get_source(&self) -> &[W] {
        &self.src
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum ShaderStage {
    Vertex = 0x01,
    Fragment = 0x02
}

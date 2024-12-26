pub trait Scene {
    #[must_use]
    fn get_type(&self) -> SceneType;
}

pub enum SceneType {
    TwoDim,
    ThreeDim,
}
